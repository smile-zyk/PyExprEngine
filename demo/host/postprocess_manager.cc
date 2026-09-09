#include "postprocess_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <utility>

#include "postprocess_instance.h"

#include "../rpc/json_rpc_peer.h"

namespace xresults
{
namespace host
{

PostprocessManager::PostprocessManager(QObject *parent) : QObject(parent)
{
    executable_ = DefaultExecutablePath();
}

PostprocessManager::~PostprocessManager()
{
    // Never leave orphan postprocess windows behind.
    StopAll();
}

QString PostprocessManager::DefaultExecutablePath()
{
    // The postprocess exe is built into the same bin directory as the host.
    const QString dir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const QString candidate = QDir(dir).filePath("postprocess.exe");
#else
    const QString candidate = QDir(dir).filePath("postprocess");
#endif
    return QFileInfo::exists(candidate) ? QFileInfo(candidate).absoluteFilePath()
                                        : QString();
}

void PostprocessManager::set_executable(const QString &path)
{
    executable_ = path;
}

// =========================================================================
// Lifecycle
// =========================================================================

QUuid PostprocessManager::Launch(const LaunchConfig &config, QString *error)
{
    if (executable_.isEmpty())
    {
        if (error)
        {
            *error = QStringLiteral("postprocess executable path is not set");
        }
        return QUuid();
    }

    const QUuid id = QUuid::createUuid();
    PostprocessInstance *instance = new PostprocessInstance(executable_, id, this);

    connect(instance, &PostprocessInstance::LogMessage, this,
            &PostprocessManager::LogMessage);
    connect(instance, &PostprocessInstance::Ready, this, [this, id]() {
        FlushQueue(id);
        Q_EMIT InstanceReady(id);
    });
    connect(instance, &PostprocessInstance::Finished, this,
            [this, id](int exit_code) {
                queued_.erase(id.toString());
                Q_EMIT InstanceFinished(id, exit_code);
            });
    connect(instance, &PostprocessInstance::NotificationReceived, this,
            [this, id](const QString &method, const QJsonObject &params) {
                auto it = notification_handlers_.find(method);
                if (it != notification_handlers_.end() && it->second)
                {
                    it->second(id, params);
                }
            });

    QString start_error;
    if (!instance->Start(config, &start_error))
    {
        if (error)
        {
            *error = start_error;
        }
        delete instance;
        return QUuid();
    }

    instances_.push_back(instance);
    Q_EMIT InstanceLaunched(id);
    return id;
}

void PostprocessManager::Stop(const QUuid &id)
{
    if (PostprocessInstance *instance = this->instance(id))
    {
        instance->Stop();
    }
    queued_.erase(id.toString());
}

void PostprocessManager::StopAll()
{
    for (PostprocessInstance *instance : instances_)
    {
        if (instance)
        {
            instance->Stop();
        }
    }
    queued_.clear();
}

int PostprocessManager::count() const
{
    return static_cast<int>(instances_.size());
}

std::vector<QUuid> PostprocessManager::ids() const
{
    std::vector<QUuid> result;
    result.reserve(instances_.size());
    for (const PostprocessInstance *instance : instances_)
    {
        if (instance)
        {
            result.push_back(instance->id());
        }
    }
    return result;
}

PostprocessInstance *PostprocessManager::instance(const QUuid &id) const
{
    for (PostprocessInstance *instance : instances_)
    {
        if (instance && instance->id() == id)
        {
            return instance;
        }
    }
    return nullptr;
}

// =========================================================================
// RPC
// =========================================================================

void PostprocessManager::SendOrQueue(const QUuid &id, const QString &method,
                                     const QJsonObject &params,
                                     ResponseCallback callback)
{
    PostprocessInstance *instance = this->instance(id);
    if (!instance)
    {
        if (callback)
        {
            callback(false, QJsonValue(),
                     QStringLiteral("no postprocess instance %1")
                         .arg(id.toString()));
        }
        return;
    }

    // A child that has not sent `ready` yet cannot receive anything -- queue
    // the command and flush it on Ready() so callers never have to wait.
    if (!instance->is_ready())
    {
        queued_[id.toString()].push_back(
            QueuedCommand{method, params, std::move(callback)});
        return;
    }

    instance->Call(method, params, std::move(callback));
}

void PostprocessManager::FlushQueue(const QUuid &id)
{
    auto it = queued_.find(id.toString());
    if (it == queued_.end())
    {
        return;
    }

    // Move out first: a callback may issue further commands, which must not
    // re-enter the vector we are iterating.
    std::vector<QueuedCommand> commands;
    commands.swap(it->second);
    queued_.erase(it);

    PostprocessInstance *instance = this->instance(id);
    if (!instance)
    {
        return;
    }
    for (QueuedCommand &command : commands)
    {
        instance->Call(command.method, command.params, std::move(command.callback));
    }
}

void PostprocessManager::Call(const QUuid &id, const QString &method,
                              const QJsonObject &params, ResponseCallback callback)
{
    SendOrQueue(id, method, params, std::move(callback));
}

void PostprocessManager::Notify(const QUuid &id, const QString &method,
                                const QJsonObject &params)
{
    if (PostprocessInstance *instance = this->instance(id))
    {
        instance->Notify(method, params);
    }
}

void PostprocessManager::RegisterMethod(
    const QString &method, std::function<QJsonValue(const QJsonObject &)> handler)
{
    // Host-side methods are registered on every instance's peer, so a child
    // can call back regardless of which instance it is.
    for (PostprocessInstance *instance : instances_)
    {
        if (instance && instance->peer())
        {
            // Wrap the QJsonObject handler in the peer's Params-based Handler
            // signature (the Params wrapper is what gives handlers typed
            // Require<>/Optional<> access).
            rpc::JsonRpcPeer::Handler peer_handler =
                [handler](const rpc::Params &p) { return handler(p.object()); };
            instance->peer()->RegisterMethod(method, peer_handler);
        }
    }
}

void PostprocessManager::OnNotification(
    const QString &method,
    std::function<void(const QUuid &id, const QJsonObject &params)> handler)
{
    notification_handlers_[method] = std::move(handler);
}

} // namespace host
} // namespace xresults
