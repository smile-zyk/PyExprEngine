#include "json_rpc_message.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace xresults
{
namespace rpc
{
namespace
{
const char kJsonRpcVersion[] = "2.0";

/// A JSON-RPC id must be a string, a number, or null.
bool IsValidId(const QJsonValue &v)
{
    return v.isString() || v.isDouble() || v.isNull();
}
} // namespace

QByteArray Encode(const QJsonObject &message)
{
    // Compact is mandatory: Indented output embeds newlines, which are the
    // frame delimiter.
    QByteArray out = QJsonDocument(message).toJson(QJsonDocument::Compact);
    out.append('\n');
    return out;
}

QJsonObject MakeRequest(const QString &method, const QJsonObject &params,
                        const QJsonValue &id)
{
    QJsonObject o;
    o["jsonrpc"] = QString::fromLatin1(kJsonRpcVersion);
    o["method"] = method;
    o["params"] = params;
    if (!id.isNull() && !id.isUndefined())
    {
        o["id"] = id;
    }
    return o;
}

QJsonObject MakeResult(const QJsonValue &id, const QJsonValue &result)
{
    QJsonObject o;
    o["jsonrpc"] = QString::fromLatin1(kJsonRpcVersion);
    o["id"] = id;
    o["result"] = result;
    return o;
}

QJsonObject MakeError(const QJsonValue &id, ErrorCode code, const QString &message,
                      const QJsonValue &data)
{
    QJsonObject err;
    err["code"] = static_cast<int>(code);
    err["message"] = message;
    if (!data.isNull() && !data.isUndefined())
    {
        err["data"] = data;
    }

    QJsonObject o;
    o["jsonrpc"] = QString::fromLatin1(kJsonRpcVersion);
    o["id"] = id;
    o["error"] = err;
    return o;
}

bool ParseRequest(const QByteArray &line, Request *request, QJsonObject *error)
{
    if (request == nullptr || error == nullptr)
    {
        return false;
    }

    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &parse_error);
    if (parse_error.error != QJsonParseError::NoError)
    {
        *error = MakeError(QJsonValue(), ErrorCode::kParseError,
                           QStringLiteral("JSON parse error: %1")
                               .arg(parse_error.errorString()));
        return false;
    }
    if (!doc.isObject())
    {
        *error = MakeError(QJsonValue(), ErrorCode::kInvalidRequest,
                           QStringLiteral("request must be a JSON object"));
        return false;
    }
    return ParseRequestObject(doc.object(), request, error);
}

bool ParseRequestObject(const QJsonObject &o, Request *request, QJsonObject *error)
{
    if (request == nullptr || error == nullptr)
    {
        return false;
    }

    // Recover the id first so even an invalid request can be answered with the
    // id the caller used.
    const QJsonValue id = o.value(QStringLiteral("id"));
    request->id = IsValidId(id) ? id : QJsonValue();

    if (GetString(o, QStringLiteral("jsonrpc")) !=
        QString::fromLatin1(kJsonRpcVersion))
    {
        *error = MakeError(request->id, ErrorCode::kInvalidRequest,
                           QStringLiteral("\"jsonrpc\" must be \"2.0\""));
        return false;
    }

    request->method = GetString(o, QStringLiteral("method"));
    if (request->method.isEmpty())
    {
        *error = MakeError(request->id, ErrorCode::kInvalidRequest,
                           QStringLiteral("\"method\" is missing or not a string"));
        return false;
    }

    // params is optional; only the by-name (object) form is supported.
    const QJsonValue params = o.value(QStringLiteral("params"));
    if (!params.isUndefined() && !params.isNull())
    {
        if (!params.isObject())
        {
            *error = MakeError(
                request->id, ErrorCode::kInvalidParams,
                QStringLiteral("\"params\" must be an object (by-name params)"));
            return false;
        }
        request->params = params.toObject();
    }
    else
    {
        request->params = QJsonObject();
    }

    return true;
}

bool ParseResponse(const QByteArray &line, Response *response, QString *parse_error)
{
    if (response == nullptr)
    {
        return false;
    }

    QJsonParseError json_error;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &json_error);
    if (json_error.error != QJsonParseError::NoError)
    {
        if (parse_error)
        {
            *parse_error = QStringLiteral("JSON parse error: %1")
                               .arg(json_error.errorString());
        }
        return false;
    }
    if (!doc.isObject())
    {
        if (parse_error)
        {
            *parse_error = QStringLiteral("response must be a JSON object");
        }
        return false;
    }
    return ParseResponseObject(doc.object(), response, parse_error);
}

bool ParseResponseObject(const QJsonObject &o, Response *response,
                         QString *parse_error)
{
    if (response == nullptr)
    {
        return false;
    }

    response->id = o.value(QStringLiteral("id"));

    const QJsonValue error_value = o.value(QStringLiteral("error"));
    if (error_value.isObject())
    {
        const QJsonObject err = error_value.toObject();
        response->is_error = true;
        response->error_code = err.value(QStringLiteral("code")).toInt();
        response->error_message = GetString(err, QStringLiteral("message"));
        response->error_data = err.value(QStringLiteral("data"));
        return true;
    }

    if (o.contains(QStringLiteral("result")))
    {
        response->is_error = false;
        response->result = o.value(QStringLiteral("result"));
        return true;
    }

    if (parse_error)
    {
        *parse_error = QStringLiteral("response has neither \"result\" nor \"error\"");
    }
    return false;
}

QString GetString(const QJsonObject &o, const QString &key, const QString &def)
{
    const QJsonValue v = o.value(key);
    return v.isString() ? v.toString() : def;
}

bool GetBool(const QJsonObject &o, const QString &key, bool def)
{
    const QJsonValue v = o.value(key);
    return v.isBool() ? v.toBool() : def;
}

QJsonArray GetArray(const QJsonObject &o, const QString &key)
{
    const QJsonValue v = o.value(key);
    return v.isArray() ? v.toArray() : QJsonArray();
}

QJsonObject GetObject(const QJsonObject &o, const QString &key)
{
    const QJsonValue v = o.value(key);
    return v.isObject() ? v.toObject() : QJsonObject();
}

QString IdToString(const QJsonValue &id)
{
    if (id.isString())
    {
        return id.toString();
    }
    if (id.isDouble())
    {
        return QString::number(static_cast<qlonglong>(id.toDouble()));
    }
    return QStringLiteral("null");
}

} // namespace rpc
} // namespace xresults
