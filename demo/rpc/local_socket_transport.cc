#include "local_socket_transport.h"

#include <QLocalServer>
#include <QLocalSocket>

#include <cstdio>

namespace xresults
{
namespace rpc
{

LocalSocketTransport::LocalSocketTransport(Role role, const QString &channel_name,
                                           QObject *parent)
    : StreamTransport(parent), role_(role), channel_name_(channel_name)
{
}

LocalSocketTransport::~LocalSocketTransport()
{
    Close();
}

bool LocalSocketTransport::Start(QString *error)
{
    if (channel_name_.isEmpty())
    {
        if (error)
        {
            *error = QStringLiteral("LocalSocketTransport needs a channel name");
        }
        return false;
    }

    if (role_ == Role::kHost)
    {
        server_ = new QLocalServer(this);
        // A previous crashed run may have left the pipe behind.
        QLocalServer::removeServer(channel_name_);
        if (!server_->listen(channel_name_))
        {
            if (error)
            {
                *error = QStringLiteral("cannot listen on %1: %2")
                             .arg(channel_name_, server_->errorString());
            }
            return false;
        }
        connect(server_, &QLocalServer::newConnection, this,
                &LocalSocketTransport::OnNewConnection);
        // The transport is "up" once we are listening; the peer arrives
        // asynchronously via OnNewConnection().
        connected_ = true;
        Q_EMIT Connected();
        return true;
    }

    // ---- child side ----------------------------------------------------
    socket_ = new QLocalSocket(this);
    connect(socket_, &QLocalSocket::connected, this, [this]() {
        connected_ = true;
        Q_EMIT Connected();
    });
    connect(socket_, &QLocalSocket::disconnected, this,
            &LocalSocketTransport::OnSocketDisconnected);
    connect(socket_, &QLocalSocket::errorOccurred, this,
            &LocalSocketTransport::OnSocketError);

    // One QLocalSocket is both the read and the write side (full duplex).
    SetDevices(socket_, socket_);
    socket_->connectToServer(channel_name_);
    return true;
}

void LocalSocketTransport::OnNewConnection()
{
    if (socket_ != nullptr || !server_)
    {
        return;  // already connected (a second connection is unexpected)
    }

    socket_ = server_->nextPendingConnection();
    if (!socket_)
    {
        return;
    }
    socket_->setParent(this);

    // One QLocalSocket is both the read and the write side (full duplex).
    SetDevices(socket_, socket_);
    connect(socket_, &QLocalSocket::disconnected, this,
            &LocalSocketTransport::OnSocketDisconnected);
    connect(socket_, &QLocalSocket::errorOccurred, this,
            &LocalSocketTransport::OnSocketError);

    connected_ = true;
    Q_EMIT Connected();

    // Only one child ever connects; stop listening so the name is freed and no
    // stray process can attach.
    server_->close();
}

void LocalSocketTransport::Close()
{
    if (socket_)
    {
        socket_->disconnectFromServer();
        socket_->deleteLater();
        socket_ = nullptr;
    }
    if (server_)
    {
        server_->close();
        QLocalServer::removeServer(channel_name_);
        server_->deleteLater();
        server_ = nullptr;
    }
    connected_ = false;
}

bool LocalSocketTransport::isConnected() const
{
    return connected_ && (socket_ != nullptr) &&
           socket_->state() == QLocalSocket::ConnectedState;
}

QString LocalSocketTransport::Describe() const
{
    return QStringLiteral("local:%1").arg(channel_name_);
}

QStringList LocalSocketTransport::ChildArguments() const
{
    // There is only one transport, so no --transport flag: the child just
    // needs the channel name to connect back to.
    return QStringList{QStringLiteral("--channel"), channel_name_};
}

void LocalSocketTransport::OnSocketDisconnected()
{
    connected_ = false;
    HandleDisconnected();
}

void LocalSocketTransport::OnSocketError()
{
    if (!socket_)
    {
        return;
    }
    // A peer that simply closed is reported through disconnected(), not here.
    if (socket_->error() == QLocalSocket::PeerClosedError)
    {
        return;
    }
    Q_EMIT Error(QStringLiteral("local socket error: %1")
                     .arg(socket_->errorString()));
}

void LocalSocketTransport::RedirectQtLoggingToStderr()
{
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &,
                              const QString &msg) {
        fprintf(stderr, "%s\n", qPrintable(msg));
        fflush(stderr);
    });
}

} // namespace rpc
} // namespace xresults
