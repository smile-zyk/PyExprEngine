#include "stream_transport.h"

#include <QIODevice>
#include <QJsonDocument>
#include <QJsonParseError>

#include "json_rpc_message.h"

namespace xresults
{
namespace rpc
{

StreamTransport::StreamTransport(QObject *parent) : Transport(parent) {}

StreamTransport::~StreamTransport() = default;

void StreamTransport::SetDevices(QIODevice *read_device, QIODevice *write_device)
{
    read_device_ = read_device;
    write_device_ = write_device;

    if (read_device_)
    {
        connect(read_device_, &QIODevice::readyRead, this,
                &StreamTransport::OnReadyRead);
        connect(read_device_, &QIODevice::readChannelFinished, this,
                &StreamTransport::OnReadChannelFinished);

        // Drain anything the peer already wrote (it may have written before we
        // connected).  Safe here because every device we register is
        // sequential (QLocalSocket); for a non-sequential one bytesAvailable()
        // is size() - pos(), which is meaningless for a pipe.
        if (read_device_->isSequential() && read_device_->bytesAvailable() > 0)
        {
            OnReadyRead();
        }
    }
}

bool StreamTransport::Send(const QJsonObject &message)
{
    if (!write_device_ || !write_device_->isWritable())
    {
        return false;
    }

    const QByteArray payload = Encode(message);
    qint64 written = 0;
    while (written < payload.size())
    {
        const qint64 n = write_device_->write(payload.constData() + written,
                                              payload.size() - written);
        if (n <= 0)
        {
            return false;
        }
        written += n;
    }

    // Pipes are buffered: without an explicit flush the peer may block waiting
    // for a request that is still sitting in our buffer.  A 0 timeout just
    // kicks the write queue without blocking the event loop.
    write_device_->waitForBytesWritten(0);
    return true;
}

void StreamTransport::FeedBytes(const QByteArray &bytes)
{
    buffer_.append(bytes);

    int newline = buffer_.indexOf('\n');
    while (newline >= 0)
    {
        QByteArray line = buffer_.left(newline);
        buffer_.remove(0, newline + 1);

        // Tolerate CRLF (a peer whose stdout is in text mode).
        if (line.endsWith('\r'))
        {
            line.chop(1);
        }
        if (!line.trimmed().isEmpty())
        {
            // Emit the parsed object as-is: the peer decides whether it is a
            // request, a notification or a response (see json_rpc_message.h).
            QJsonParseError parse_error;
            const QJsonDocument doc = QJsonDocument::fromJson(line, &parse_error);
            if (parse_error.error == QJsonParseError::NoError && doc.isObject())
            {
                Q_EMIT MessageReceived(doc.object());
            }
            else
            {
                Q_EMIT Error(QStringLiteral("unparsable message: %1")
                                 .arg(parse_error.errorString()));
            }
        }
        newline = buffer_.indexOf('\n');
    }
}

void StreamTransport::OnReadyRead()
{
    if (!read_device_)
    {
        return;
    }
    FeedBytes(read_device_->readAll());
}

void StreamTransport::OnReadChannelFinished()
{
    // Flush a final message that was not newline-terminated before EOF.
    if (!buffer_.trimmed().isEmpty())
    {
        QByteArray line = buffer_;
        buffer_.clear();
        if (line.endsWith('\r'))
        {
            line.chop(1);
        }
        FeedBytes(line + "\n");
    }
    HandleDisconnected();
}

void StreamTransport::HandleDisconnected()
{
    if (disconnected_emitted_)
    {
        return;
    }
    disconnected_emitted_ = true;
    Q_EMIT Disconnected();
}

} // namespace rpc
} // namespace xresults
