#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>
#include <QUuid>

#include <functional>
#include <map>
#include <vector>

#include "launch_config.h"

namespace xresults
{
namespace rpc
{
class JsonRpcPeer;
}

namespace host
{

class PostprocessInstance;

// =========================================================================
// PostprocessManager -- the library a host application uses to launch and
// drive postprocess instances.  Create one, Launch() as many as you want, then
// send commands by id.  Owns every instance, so destroying it stops them all.
//
// There are deliberately NO convenience wrappers like AddEquation()/Raise():
// a call must look like a cross-process call, because it is one -- it can fail
// and its result arrives asynchronously.  Call() is the single RPC entry
// point, which also means adding a child-side method needs no host change.
// =========================================================================

class PostprocessManager : public QObject
{
    Q_OBJECT

  public:
    /// (ok, result, error) -- see PostprocessInstance::Call.
    using ResponseCallback = std::function<void(
        bool ok, const QJsonValue &result, const QString &error)>;

    explicit PostprocessManager(QObject *parent = nullptr);
    ~PostprocessManager() override;

    // ---- configuration ---------------------------------------------------

    /// Absolute path of the `postprocess` executable.  The constructor seeds
    /// it with DefaultExecutablePath().
    void set_executable(const QString &path);
    const QString &executable() const
    {
        return executable_;
    }

    /// The `postprocess` executable next to this application (same bin dir).
    /// Empty when it is not there.
    static QString DefaultExecutablePath();

    // ---- lifecycle -------------------------------------------------------

    /// Launch a new instance.  Returns its id, or a null QUuid on failure
    /// (see @p error).
    QUuid Launch(const LaunchConfig &config, QString *error = nullptr);

    /// Stop one instance (graceful shutdown, then terminate / kill).
    void Stop(const QUuid &id);

    /// Stop every instance.  Called by the destructor.
    void StopAll();

    int count() const;
    std::vector<QUuid> ids() const;

    /// The instance with @p id, or nullptr when there is none.
    PostprocessInstance *instance(const QUuid &id) const;

    // ---- RPC -------------------------------------------------------------

    /// The single RPC entry point.  Commands issued before the child is ready
    /// are queued and flushed on Ready(), so Launch() followed immediately by
    /// Call() works.
    void Call(const QUuid &id, const QString &method, const QJsonObject &params,
              ResponseCallback callback = ResponseCallback());

    /// Send a notification (fire and forget).
    void Notify(const QUuid &id, const QString &method, const QJsonObject &params);

    /// Register a method the child can call back into (symmetric peer).
    void RegisterMethod(const QString &method,
                        std::function<QJsonValue(const QJsonObject &)> handler);

    /// Register a handler for a child notification.
    void OnNotification(
        const QString &method,
        std::function<void(const QUuid &id, const QJsonObject &params)> handler);

  Q_SIGNALS:
    void InstanceLaunched(const QUuid &id);
    void InstanceReady(const QUuid &id);
    void InstanceFinished(const QUuid &id, int exit_code);
    void LogMessage(const QString &text);

  private:
    /// Send now, or queue when the instance is not ready yet.
    void SendOrQueue(const QUuid &id, const QString &method,
                     const QJsonObject &params, ResponseCallback callback);

    /// Flush the queued commands of an instance that just became ready.
    void FlushQueue(const QUuid &id);

    struct QueuedCommand
    {
        QString method;
        QJsonObject params;
        ResponseCallback callback;
    };

    QString executable_;
    std::vector<PostprocessInstance *> instances_;
    std::map<QString, std::vector<QueuedCommand>> queued_;  // key: id string
    std::map<QString, std::function<void(const QUuid &, const QJsonObject &)>>
        notification_handlers_;
};

} // namespace host
} // namespace xresults
