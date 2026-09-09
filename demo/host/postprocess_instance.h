#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QUuid>

#include <functional>

#include "launch_config.h"

namespace xresults
{
namespace rpc
{
class Transport;
class JsonRpcPeer;
}

namespace host
{

// =========================================================================
// PostprocessInstance -- ONE launched `postprocess` child process plus the
// JSON-RPC 2.0 peer that talks to it.  Owns the QProcess and the transport.
//
// Stop() asks the child to quit politely (`system.shutdown`, so it runs its
// Python / REL teardown), then escalates to terminate() / kill().  The
// destructor escalates immediately (no event loop left to wait on), so a host
// crash never leaves an orphan window behind.
//
// A freshly launched child needs a moment to connect and send `ready`, so
// commands issued before that are queued and flushed on Ready() -- Launch()
// followed immediately by Call() works.
// =========================================================================

class PostprocessInstance : public QObject
{
    Q_OBJECT

  public:
    /// (ok, result, error) -- on failure `result` is null and `error` is set.
    using ResponseCallback = std::function<void(
        bool ok, const QJsonValue &result, const QString &error)>;

    /// @param program  absolute path of the `postprocess` executable.
    /// @param id       host-side identity.
    PostprocessInstance(const QString &program, const QUuid &id,
                        QObject *parent = nullptr);
    ~PostprocessInstance() override;

    /// Start the child.  Returns false (and fills @p error) when the config
    /// temp file cannot be written or the process fails to start.  The
    /// connection itself is asynchronous -- wait for Ready() before sending.
    bool Start(const LaunchConfig &config, QString *error = nullptr);

    /// Ask the child to exit (graceful shutdown, then terminate / kill).
    /// Blocks until the process is gone (or the grace periods expire).
    void Stop();

    /// Send a request; @p callback is invoked with the response (or
    /// immediately with an error when the child is not reachable).
    void Call(const QString &method, const QJsonObject &params,
              ResponseCallback callback = ResponseCallback());

    /// Send a notification (fire and forget, no response).
    void Notify(const QString &method, const QJsonObject &params);

    // ---- state ---------------------------------------------------------

    const QUuid &id() const
    {
        return id_;
    }
    const QString &title() const
    {
        return title_;
    }
    /// True once the child sent its `ready` notification.
    bool is_ready() const
    {
        return is_ready_;
    }
    bool is_running() const;
    qint64 pid() const;

    /// Direct access for hosts that need raw calls (rarely needed).
    rpc::JsonRpcPeer *peer() const
    {
        return peer_;
    }

  Q_SIGNALS:
    /// The child sent `ready` -- commands will now succeed.
    void Ready();

    /// The child process exited (crash or graceful).
    void Finished(int exit_code);

    /// Human-readable log line (RPC traffic, stderr, lifecycle).
    void LogMessage(const QString &text);

    /// The child sent a notification (e.g. `status_changed`).
    void NotificationReceived(const QString &method, const QJsonObject &params);

  private:
    void OnReadyNotification(const QString &method, const QJsonObject &params);
    void OnStandardError();
    void OnProcessFinished(int exit_code, QProcess::ExitStatus status);
    void OnProcessError(QProcess::ProcessError error);
    void OnTransportDisconnected();

    /// Fail every outstanding request (the child is gone).
    void FailPending(const QString &reason);

    QString program_;
    QUuid id_;
    QString title_;

    QProcess *process_ = nullptr;
    rpc::Transport *transport_ = nullptr;
    rpc::JsonRpcPeer *peer_ = nullptr;

    /// Temp config file passed via --config; removed when the child exits.
    QString config_file_;

    bool is_ready_ = false;
    /// Set by Stop() so a normal exit is not reported as a crash.
    bool stopping_ = false;
};

} // namespace host
} // namespace xresults
