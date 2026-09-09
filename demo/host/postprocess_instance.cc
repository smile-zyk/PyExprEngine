#include "postprocess_instance.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

#include "../rpc/json_rpc_peer.h"
#include "../rpc/local_socket_transport.h"
#include "../rpc/transport.h"

namespace xresults
{
namespace host
{
namespace
{
/// Grace period given to a child after `system.shutdown` / terminate() before
/// the next escalation step.
const int kShutdownGraceMs = 3000;
const int kTerminateGraceMs = 2000;

/// Short display form of an instance id.  QUuid::toString() wraps the uuid in
/// braces ("{470167f4-...}"), so taking left(8) of it yields "{470167f" --
/// strip the braces first.
QString ShortId(const QUuid &id)
{
    const QString text = id.toString();
    const QString bare = text.startsWith('{') ? text.mid(1) : text;
    return bare.left(8);
}
} // namespace

PostprocessInstance::PostprocessInstance(const QString &program, const QUuid &id,
                                         QObject *parent)
    : QObject(parent), program_(program), id_(id)
{
}

PostprocessInstance::~PostprocessInstance()
{
    // No event loop is available any more, so escalate immediately rather than
    // waiting for a graceful shutdown that can never be delivered.  A short
    // bounded wait is fine here (the process is being torn down anyway).
    if (process_ && process_->state() != QProcess::NotRunning)
    {
        process_->terminate();
        if (!process_->waitForFinished(kTerminateGraceMs))
        {
            process_->kill();
            process_->waitForFinished(kTerminateGraceMs);
        }
    }
    if (!config_file_.isEmpty())
    {
        QFile::remove(config_file_);
    }
}

bool PostprocessInstance::Start(const LaunchConfig &config, QString *error)
{
    if (process_ != nullptr)
    {
        if (error)
        {
            *error = QStringLiteral("instance was already started");
        }
        return false;
    }

    title_ = config.title.isEmpty()
                 ? QStringLiteral("Postprocess %1").arg(ShortId(id_))
                 : config.title;

    // ---- write the startup config to a temp file -----------------------
    LaunchConfig effective = config;
    effective.title = title_;

    const QString temp_dir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    config_file_ = QDir(temp_dir).filePath(
        QStringLiteral("postprocess_launch_%1.json").arg(id_.toString()));

    QFile file(config_file_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (error)
        {
            *error = QStringLiteral("cannot write launch config %1: %2")
                         .arg(config_file_, file.errorString());
        }
        config_file_.clear();
        return false;
    }
    file.write(QJsonDocument(effective.ToJson()).toJson(QJsonDocument::Indented));
    file.close();

    // ---- start the child ------------------------------------------------
    process_ = new QProcess(this);
    // The protocol runs on a local socket, so the child's stdout is free for
    // its own output; stderr is read separately and mirrored into the log.
    process_->setProcessChannelMode(QProcess::SeparateChannels);
    const QString working_dir = config.working_directory.isEmpty()
                                    ? QFileInfo(program_).absolutePath()
                                    : config.working_directory;
    process_->setWorkingDirectory(working_dir);

    connect(process_, &QProcess::readyReadStandardError, this,
            &PostprocessInstance::OnStandardError);
    connect(process_,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                &QProcess::finished),
            this, &PostprocessInstance::OnProcessFinished);
    connect(process_, &QProcess::errorOccurred, this,
            &PostprocessInstance::OnProcessError);

    // ---- transport -----------------------------------------------------
    // The server must be listening BEFORE the child starts, otherwise the
    // child's connect() races our listen() and fails.
    const QString channel = QStringLiteral("xequation.postprocess.%1")
                                .arg(id_.toString());
    transport_ = new rpc::LocalSocketTransport(rpc::Transport::Role::kHost,
                                               channel, this);
    QString transport_error;
    if (!transport_->Start(&transport_error))
    {
        if (error)
        {
            *error = transport_error;
        }
        return false;
    }

    peer_ = new rpc::JsonRpcPeer(transport_, this);
    connect(peer_, &rpc::JsonRpcPeer::NotificationReceived, this,
            &PostprocessInstance::OnReadyNotification);
    connect(transport_, &rpc::Transport::Disconnected, this,
            &PostprocessInstance::OnTransportDisconnected);

    // Transport-specific arguments + the config file.
    QStringList args = QStringList{QStringLiteral("--rpc")};
    args << transport_->ChildArguments();
    args << QStringLiteral("--config") << config_file_;

    // start() is asynchronous: waitForStarted() would block the GUI thread for
    // up to 5s (and the child needs a moment to load Qt + Python anyway).
    process_->start(program_, args);
    return true;
}

void PostprocessInstance::OnReadyNotification(const QString &method,
                                              const QJsonObject &params)
{
    if (method == QStringLiteral("ready"))
    {
        is_ready_ = true;
        const QString reported_title =
            rpc::GetString(params, QStringLiteral("title"));
        if (!reported_title.isEmpty())
        {
            title_ = reported_title;
        }
        Q_EMIT LogMessage(QStringLiteral("[%1] ready").arg(ShortId(id_)));
        Q_EMIT Ready();
    }
    Q_EMIT NotificationReceived(method, params);
}

void PostprocessInstance::Stop()
{
    if (!process_ || process_->state() == QProcess::NotRunning)
    {
        return;
    }
    stopping_ = true;

    // 1. Polite: the child closes its window and runs its own teardown.
    Notify(QStringLiteral("system.shutdown"), QJsonObject());

    // Escalation is driven by timers, not by blocking waits -- Stop() runs on
    // the GUI thread and must return immediately.
    QTimer::singleShot(kShutdownGraceMs, this, [this]() {
        if (!process_ || process_->state() == QProcess::NotRunning)
        {
            return;
        }
        // 2. terminate() -> WM_CLOSE on Windows / SIGTERM elsewhere.
        Q_EMIT LogMessage(
            QStringLiteral("[%1] did not answer shutdown, terminating")
                .arg(ShortId(id_)));
        process_->terminate();

        QTimer::singleShot(kTerminateGraceMs, this, [this]() {
            if (!process_ || process_->state() == QProcess::NotRunning)
            {
                return;
            }
            // 3. Last resort.
            Q_EMIT LogMessage(
                QStringLiteral("[%1] killing").arg(ShortId(id_)));
            process_->kill();
        });
    });
}

void PostprocessInstance::Call(const QString &method, const QJsonObject &params,
                               ResponseCallback callback)
{
    if (!peer_ || !is_running())
    {
        if (callback)
        {
            callback(false, QJsonValue(),
                     QStringLiteral("postprocess %1 is not running")
                         .arg(ShortId(id_)));
        }
        return;
    }
    peer_->Call(method, params, std::move(callback));
}

void PostprocessInstance::Notify(const QString &method, const QJsonObject &params)
{
    if (!peer_ || !is_running())
    {
        return;
    }
    peer_->Notify(method, params);
}

bool PostprocessInstance::is_running() const
{
    return process_ && process_->state() != QProcess::NotRunning;
}

qint64 PostprocessInstance::pid() const
{
    return process_ ? process_->processId() : 0;
}

void PostprocessInstance::OnStandardError()
{
    if (!process_)
    {
        return;
    }
    const QByteArray data = process_->readAllStandardError();
    const QStringList lines = QString::fromLocal8Bit(data).split(
        '\n', Qt::SkipEmptyParts);
    for (const QString &text : lines)
    {
        Q_EMIT LogMessage(QStringLiteral("[%1] stderr: %2")
                              .arg(ShortId(id_))
                              .arg(text.trimmed()));
    }
}

void PostprocessInstance::OnProcessFinished(int exit_code,
                                            QProcess::ExitStatus status)
{
    is_ready_ = false;

    const QString how = (status == QProcess::CrashExit)
                            ? QStringLiteral("crashed")
                            : QStringLiteral("exited");
    Q_EMIT LogMessage(QStringLiteral("[%1] %2 (code %3)")
                          .arg(ShortId(id_))
                          .arg(how)
                          .arg(exit_code));

    if (peer_)
    {
        peer_->FailPending(
            QStringLiteral("postprocess %1 %2").arg(ShortId(id_)).arg(how));
    }

    if (!config_file_.isEmpty())
    {
        QFile::remove(config_file_);
        config_file_.clear();
    }

    Q_EMIT Finished(exit_code);
}

void PostprocessInstance::OnProcessError(QProcess::ProcessError error)
{
    // A Crashed error right after Stop() is the expected result of kill().
    if (stopping_ && error == QProcess::Crashed)
    {
        return;
    }
    Q_EMIT LogMessage(QStringLiteral("[%1] process error: %2")
                          .arg(ShortId(id_))
                          .arg(process_ ? process_->errorString()
                                        : QStringLiteral("unknown")));
}

void PostprocessInstance::OnTransportDisconnected()
{
    is_ready_ = false;
    if (peer_)
    {
        peer_->FailPending(
            QStringLiteral("postprocess %1 disconnected").arg(ShortId(id_)));
    }
}

} // namespace host
} // namespace xresults
