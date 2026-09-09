#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

namespace xresults
{
namespace rpc
{

// =========================================================================
// Transport -- how JSON-RPC messages get from one process to another.
//
// Message-level (Send(QJsonObject) / MessageReceived), not byte-level: MCP's
// HTTP transport has no persistent byte stream, so exposing QIODevice here
// would exclude it.  Framing lives in the stream transports, not here.
//
// Disconnected() normalizes "peer is gone" across transports.
//
// Threading: created on the GUI thread and driven by its event loop, so all
// signals run there and handlers can touch widgets directly.
// =========================================================================

class Transport : public QObject
{
    Q_OBJECT

  public:
    /// Which side of the connection this transport is on.
    enum class Role
    {
        kHost,   ///< the process that starts the peer
        kChild,  ///< the process that was started
    };

    explicit Transport(QObject *parent = nullptr) : QObject(parent) {}
    ~Transport() override = default;

    /// Bring the transport up.  Returns false and fills @p error on failure.
    virtual bool Start(QString *error = nullptr) = 0;

    /// Tear it down (idempotent).
    virtual void Close() = 0;

    /// Send one complete message.  Returns false when the peer is gone.
    virtual bool Send(const QJsonObject &message) = 0;

    virtual bool isConnected() const = 0;

    /// Human-readable description for logs (e.g. "local:xyz").
    virtual QString Describe() const = 0;

    /// Command-line arguments the child needs to use this transport.
    /// Only meaningful on the host side (the child parses them itself).
    virtual QStringList ChildArguments() const = 0;

  Q_SIGNALS:
    void Connected();
    void MessageReceived(const QJsonObject &message);
    /// The peer is gone (closed, exited, or crashed).
    void Disconnected();
    void Error(const QString &text);
};

} // namespace rpc
} // namespace xresults
