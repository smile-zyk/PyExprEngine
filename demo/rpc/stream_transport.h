#pragma once

#include <QByteArray>
#include <QObject>

#include "transport.h"

class QIODevice;

namespace xresults
{
namespace rpc
{

// =========================================================================
// StreamTransport -- newline-delimited JSON framing over a QIODevice pair.
//
// Base class of byte-stream transports; owns the framing so subclasses only
// answer "where does the QIODevice come from?".
//
// Reads: buffer + split on '\n', so a message split across chunks is
// reassembled.  Writes: serialize + '\n' + flush (pipes are buffered; without
// the flush the peer can stall waiting for a request still in our buffer).
//
// Subclass contract: call SetDevices() once the devices exist, emit
// Connected() when up, call HandleDisconnected() when the peer goes away.
// =========================================================================

class StreamTransport : public Transport
{
    Q_OBJECT

  public:
    explicit StreamTransport(QObject *parent = nullptr);
    ~StreamTransport() override;

    bool Send(const QJsonObject &message) override;

  public:
    /// Install the devices and hook up readyRead.  May be called once.
    void SetDevices(QIODevice *read_device, QIODevice *write_device);

    /// Feed raw bytes into the framer (used by transports that read on a
    /// worker thread instead of via readyRead).
    void FeedBytes(const QByteArray &bytes);

    /// Emit Disconnected() (idempotent per connection).
    void HandleDisconnected();

  protected:

    QIODevice *read_device() const
    {
        return read_device_;
    }
    QIODevice *write_device() const
    {
        return write_device_;
    }

  private:
    void OnReadyRead();
    void OnReadChannelFinished();

    QIODevice *read_device_ = nullptr;
    QIODevice *write_device_ = nullptr;

    /// Bytes received but not yet terminated by '\n'.
    QByteArray buffer_;
    bool disconnected_emitted_ = false;
};

} // namespace rpc
} // namespace xresults
