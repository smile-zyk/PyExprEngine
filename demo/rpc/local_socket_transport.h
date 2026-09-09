#pragma once

#include <QObject>
#include <QString>

#include "stream_transport.h"

class QLocalServer;
class QLocalSocket;

namespace xresults
{
namespace rpc
{

// =========================================================================
// LocalSocketTransport -- JSON-RPC over a QLocalSocket (named pipe on Windows,
// Unix domain socket elsewhere).
//
// Host listens on a unique name and accepts the child's connection; the child
// connects back to it.  The name travels as `--channel <name>`.
//
// Chosen over the child's stdin/stdout because QLocalSocket is a sequential
// device (readyRead works, no reader thread), `disconnected` is a reliable
// "peer is gone" signal, and the child's stdout stays free for its own output
// (Python plugins print to it).
// =========================================================================

class LocalSocketTransport : public StreamTransport
{
    Q_OBJECT

  public:
    /// @param role     kHost listens on @p channel_name; kChild connects to it.
    /// @param channel_name  the local server name (required).
    explicit LocalSocketTransport(Role role, const QString &channel_name,
                                  QObject *parent = nullptr);
    ~LocalSocketTransport() override;

    bool Start(QString *error = nullptr) override;
    void Close() override;
    bool isConnected() const override;
    QString Describe() const override;
    QStringList ChildArguments() const override;

    /// Route Qt's logging (qWarning / qDebug / ...) to stderr.  The protocol
    /// runs on a socket so stdout is not at risk, but keeping diagnostics on
    /// stderr keeps the console clean and matches what the host reads on a
    /// separate channel.  Call once at child startup, before any logging.
    static void RedirectQtLoggingToStderr();

  private:
    void OnNewConnection();
    void OnSocketDisconnected();
    void OnSocketError();

    Role role_ = Role::kChild;
    QString channel_name_;

    QLocalServer *server_ = nullptr;  // host side
    QLocalSocket *socket_ = nullptr;  // both sides (host: accepted)

    bool connected_ = false;
};

} // namespace rpc
} // namespace xresults
