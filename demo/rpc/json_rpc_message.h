#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace xresults
{
namespace rpc
{

// =========================================================================
// JSON-RPC 2.0 message helpers (Qt JSON -- the UI layer already links Qt).
//
// Classification: has "method" -> Request (has id) or Notification (no id);
// has "result"/"error" -> Response.  A Notification is a Request without an
// `id` and gets no reply.  Note `"id": null` IS a valid request id, which is
// why MakeRequest omits the member entirely for notifications.
// =========================================================================

/// Standard JSON-RPC 2.0 error codes (spec section 5.1).
enum class ErrorCode : int
{
    kParseError = -32700,
    kInvalidRequest = -32600,
    kMethodNotFound = -32601,
    kInvalidParams = -32602,
    kInternalError = -32603,
    /// Implementation-defined server error (-32000..-32099 is reserved for
    /// exactly this): "the request was well-formed but could not be done"
    /// (unknown dataset, bad equation name, ...).
    kApplicationError = -32000,
};

struct Request
{
    QJsonValue id;
    QString method;
    QJsonObject params;

    bool is_notification() const
    {
        return id.isNull() || id.isUndefined();
    }
};

struct Response
{
    QJsonValue id;
    bool is_error = false;
    QJsonValue result;
    int error_code = 0;
    QString error_message;
    QJsonValue error_data;
};

// ---- encode / decode -----------------------------------------------------

/// Serialize one message (compact, single line) + '\n' frame terminator.
QByteArray Encode(const QJsonObject &message);

/// Build a request.  A null/undefined `id` produces a notification (no `id`).
QJsonObject MakeRequest(const QString &method, const QJsonObject &params,
                        const QJsonValue &id);

QJsonObject MakeResult(const QJsonValue &id, const QJsonValue &result);

QJsonObject MakeError(const QJsonValue &id, ErrorCode code, const QString &message,
                      const QJsonValue &data = QJsonValue());

/// Parse one de-framed line into a Request.  On failure fills `error` with a
/// ready-to-send JSON-RPC error object (carrying the recovered id, or null).
bool ParseRequest(const QByteArray &line, Request *request, QJsonObject *error);

/// Parse one de-framed line into a Response.
bool ParseResponse(const QByteArray &line, Response *response, QString *parse_error);

/// Same as ParseRequest but from an already-parsed object (the transport hands
/// us QJsonObject, so re-serializing just to parse again would be wasteful).
bool ParseRequestObject(const QJsonObject &o, Request *request, QJsonObject *error);

/// Same as ParseResponse but from an already-parsed object.
bool ParseResponseObject(const QJsonObject &o, Response *response,
                         QString *parse_error);

// ---- conveniences --------------------------------------------------------

QString GetString(const QJsonObject &o, const QString &key,
                  const QString &def = QString());
bool GetBool(const QJsonObject &o, const QString &key, bool def = false);
QJsonArray GetArray(const QJsonObject &o, const QString &key);
QJsonObject GetObject(const QJsonObject &o, const QString &key);

/// Printable form of an id (for logging / error text).
QString IdToString(const QJsonValue &id);

} // namespace rpc
} // namespace xresults
