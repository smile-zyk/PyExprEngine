#include "json_rpc_peer.h"

#include <utility>

#include "transport.h"

namespace xresults
{
namespace rpc
{

// Defined here (not in params.h) so the header stays dependency-light.
void ThrowInvalidParams(const QString &key, const char *type_name)
{
    throw JsonRpcError(ErrorCode::kInvalidParams,
                       QStringLiteral("param \"%1\" is required and must be a %2")
                           .arg(key)
                           .arg(QString::fromLatin1(type_name)));
}

JsonRpcPeer::JsonRpcPeer(Transport *transport, QObject *parent)
    : QObject(parent), transport_(transport)
{
    if (transport_)
    {
        connect(transport_, &Transport::MessageReceived, this,
                &JsonRpcPeer::OnMessageReceived);
    }
}

JsonRpcPeer::~JsonRpcPeer() = default;

// =========================================================================
// Serving
// =========================================================================

void JsonRpcPeer::RegisterMethod(const QString &method, Handler handler)
{
    MethodSpec spec;
    spec.name = method;
    spec.handler = std::move(handler);
    specs_[method] = spec;
}

void JsonRpcPeer::RegisterMethod(const MethodSpec &spec)
{
    specs_[spec.name] = spec;
}

QStringList JsonRpcPeer::MethodNames() const
{
    QStringList names;
    for (const auto &entry : specs_)
    {
        names.append(entry.first);  // std::map -> already sorted
    }
    return names;
}

QJsonObject JsonRpcPeer::MethodGroups() const
{
    QJsonObject groups;
    for (const auto &entry : specs_)
    {
        const QString module = entry.second.Module();
        QJsonArray list = groups.value(module).toArray();
        list.append(entry.first);
        groups[module] = list;
    }
    return groups;
}

const MethodSpec *JsonRpcPeer::FindSpec(const QString &method) const
{
    auto it = specs_.find(method);
    return it == specs_.end() ? nullptr : &it->second;
}

// =========================================================================
// Calling
// =========================================================================

qint64 JsonRpcPeer::Call(const QString &method, const QJsonObject &params,
                         ResponseCallback callback)
{
    if (!transport_ || !transport_->isConnected())
    {
        if (callback)
        {
            callback(false, QJsonValue(),
                     QStringLiteral("transport is not connected"));
        }
        return 0;
    }

    const qint64 request_id = next_request_id_++;
    if (callback)
    {
        pending_[request_id] = std::move(callback);
    }

    if (!transport_->Send(MakeRequest(method, params,
                                      QJsonValue(static_cast<double>(request_id)))))
    {
        auto it = pending_.find(request_id);
        if (it != pending_.end())
        {
            ResponseCallback cb = it->second;
            pending_.erase(it);
            cb(false, QJsonValue(), QStringLiteral("cannot write to transport"));
        }
        return request_id;
    }
    return request_id;
}

void JsonRpcPeer::Notify(const QString &method, const QJsonObject &params)
{
    if (!transport_ || !transport_->isConnected())
    {
        return;
    }
    // A notification carries no id -> MakeRequest omits the member.
    transport_->Send(MakeRequest(method, params, QJsonValue()));
}

void JsonRpcPeer::FailPending(const QString &reason)
{
    // Move out first: a callback may issue further calls, which must not
    // re-enter the map we are iterating.
    std::map<qint64, ResponseCallback> pending;
    pending.swap(pending_);
    for (auto &entry : pending)
    {
        if (entry.second)
        {
            entry.second(false, QJsonValue(), reason);
        }
    }
}

int JsonRpcPeer::pending_count() const
{
    return static_cast<int>(pending_.size());
}

// =========================================================================
// Dispatch
// =========================================================================

void JsonRpcPeer::OnMessageReceived(const QJsonObject &message)
{
    // Classify by `method` vs `result`/`error` -- NOT by `id`, because a
    // response carries an id too.
    if (message.contains(QStringLiteral("method")))
    {
        Request request;
        QJsonObject parse_error;
        if (!ParseRequestObject(message, &request, &parse_error))
        {
            if (transport_)
            {
                transport_->Send(parse_error);
            }
            return;
        }
        DispatchRequest(request);
        return;
    }

    Response response;
    QString parse_error;
    if (ParseResponseObject(message, &response, &parse_error))
    {
        DispatchResponse(response);
        return;
    }

    Q_EMIT ProtocolError(parse_error);
}

void JsonRpcPeer::DispatchRequest(const Request &request)
{
    const bool is_notification = request.is_notification();

    // A notification is a request the peer does not want answered.  Surface it
    // to the owner regardless of whether a handler is registered -- `ready`
    // and `status_changed` have no handler on the host side, and without this
    // emit the host would never learn the child came up.
    if (is_notification)
    {
        Q_EMIT NotificationReceived(request.method, request.params);
    }

    auto it = specs_.find(request.method);
    if (it == specs_.end())
    {
        if (!is_notification && transport_)
        {
            transport_->Send(MakeError(
                request.id, ErrorCode::kMethodNotFound,
                QStringLiteral("unknown method: %1").arg(request.method)));
        }
        return;
    }

    QJsonValue result;
    try
    {
        result = it->second.handler(Params(request.params));
    }
    catch (const JsonRpcError &e)
    {
        if (!is_notification && transport_)
        {
            transport_->Send(
                MakeError(request.id, e.code(), e.message(), e.data()));
        }
        return;
    }
    catch (const std::exception &e)
    {
        if (!is_notification && transport_)
        {
            transport_->Send(MakeError(request.id, ErrorCode::kApplicationError,
                                       QString::fromUtf8(e.what())));
        }
        return;
    }
    catch (...)
    {
        if (!is_notification && transport_)
        {
            transport_->Send(MakeError(request.id, ErrorCode::kInternalError,
                                       QStringLiteral("unknown error")));
        }
        return;
    }

    // A notification never gets a response, even a successful one.
    if (!is_notification && transport_)
    {
        transport_->Send(MakeResult(request.id, result));
    }
}

void JsonRpcPeer::DispatchResponse(const Response &response)
{
    const qint64 response_id = static_cast<qint64>(response.id.toDouble());
    auto it = pending_.find(response_id);
    if (it == pending_.end())
    {
        // Nothing was waiting -- a fire-and-forget Call() or a duplicate.
        return;
    }

    ResponseCallback callback = it->second;
    pending_.erase(it);

    if (response.is_error)
    {
        callback(false, QJsonValue(),
                 QStringLiteral("%1 (code %2)")
                     .arg(response.error_message)
                     .arg(response.error_code));
    }
    else
    {
        callback(true, response.result, QString());
    }
}

} // namespace rpc
} // namespace xresults
