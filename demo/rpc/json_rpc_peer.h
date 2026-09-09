#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>
#include <QStringList>

#include <exception>
#include <functional>
#include <map>

#include "json_rpc_message.h"
#include "method_spec.h"

namespace xresults
{
namespace rpc
{

class Transport;

// =========================================================================
// JsonRpcPeer -- one end of a JSON-RPC 2.0 connection.
//
// JSON-RPC 2.0 is symmetric, so this implements both halves: the postprocess
// child serves dataset.add / window.raise / ..., the host only sends (but
// could serve host.* methods with no protocol change).
//
//   RegisterMethod()  serve incoming requests / notifications
//   Call()            send a request, get a callback on the response
//   Notify()          send a notification (no response)
//
// Handlers are std::function<QJsonValue(const Params&)>.  Returning normally
// produces `result`; throwing JsonRpcError reports that code; throwing any
// other std::exception reports kApplicationError.  They run synchronously on
// the transport's thread (the GUI thread), so they must stay short.
//
// Notifications (no `id`) are dispatched like requests but produce no
// response, and handler errors are dropped (per the spec).
// =========================================================================

/// Throw this from a handler to control the reported JSON-RPC error.
class JsonRpcError : public std::exception
{
  public:
    JsonRpcError(ErrorCode code, const QString &message,
                 const QJsonValue &data = QJsonValue())
        : code_(code), message_(message), data_(data), utf8_(message.toUtf8())
    {
    }

    const char *what() const noexcept override
    {
        return utf8_.constData();
    }

    ErrorCode code() const
    {
        return code_;
    }
    const QString &message() const
    {
        return message_;
    }
    const QJsonValue &data() const
    {
        return data_;
    }

  private:
    ErrorCode code_;
    QString message_;
    QJsonValue data_;
    /// what() must return a stable pointer -- keep the UTF-8 bytes alive.
    QByteArray utf8_;
};

class JsonRpcPeer : public QObject
{
    Q_OBJECT

  public:
    using Handler = std::function<QJsonValue(const Params &)>;
    /// (ok, result, error) -- on failure `result` is null and `error` is set.
    using ResponseCallback = std::function<void(
        bool ok, const QJsonValue &result, const QString &error)>;

    /// @param transport  the message transport; NOT owned (usually parented to
    ///                   the caller).
    explicit JsonRpcPeer(Transport *transport, QObject *parent = nullptr);
    ~JsonRpcPeer() override;

    // ---- serving --------------------------------------------------------

    /// Register (or replace) the handler for `method`.
    void RegisterMethod(const QString &method, Handler handler);

    /// Register with metadata (needed for MCP tools/list).
    void RegisterMethod(const MethodSpec &spec);

    /// Names of every registered method (sorted).
    QStringList MethodNames() const;

    /// Method names grouped by their `module.` prefix (for list_methods).
    QJsonObject MethodGroups() const;

    /// The registered spec, or nullptr when the method has none / is unknown.
    const MethodSpec *FindSpec(const QString &method) const;

    // ---- calling --------------------------------------------------------

    /// Send a request; @p callback is invoked with the response (or
    /// immediately with an error when the transport is not writable).
    /// Returns the request id (useful for logging).
    qint64 Call(const QString &method, const QJsonObject &params,
                ResponseCallback callback = ResponseCallback());

    /// Send a notification (fire and forget, no response).
    void Notify(const QString &method, const QJsonObject &params);

    /// Fail every outstanding request (the peer is gone).  Called by the owner
    /// when the transport dies so no caller waits forever.
    void FailPending(const QString &reason);

    int pending_count() const;

  Q_SIGNALS:
    /// The peer sent a notification (a request with no id), e.g. `ready`.
    void NotificationReceived(const QString &method, const QJsonObject &params);

    /// A message arrived that is neither a parsable response nor a request.
    void ProtocolError(const QString &text);

  private:
    void OnMessageReceived(const QJsonObject &message);
    void DispatchRequest(const Request &request);
    void DispatchResponse(const Response &response);

    Transport *transport_ = nullptr;
    /// std::map (not unordered) so MethodNames() is sorted for free.
    std::map<QString, MethodSpec> specs_;

    qint64 next_request_id_ = 1;
    std::map<qint64, ResponseCallback> pending_;
};

} // namespace rpc
} // namespace xresults
