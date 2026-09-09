#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

namespace xresults
{
namespace gui
{
class PostprocessWidget;
}

namespace rpc
{

class Transport;
class JsonRpcPeer;

// =========================================================================
// PostprocessRpcService -- the JSON-RPC 2.0 surface of a postprocess process.
//
// Owns the transport + peer and assembles the method modules; each module is
// one registration call, so adding a group of methods is one line.
//
// NotifyReady() is sent once the window is up.  `system.shutdown` closes the
// window and quits.  When the transport reports Disconnected (host died) the
// service quits too, so no orphan window is left behind.
// =========================================================================

class PostprocessRpcService : public QObject
{
    Q_OBJECT

  public:
    /// @param widget    the post-processing UI this service drives (not owned).
    /// @param transport the message transport (owned by this service).
    explicit PostprocessRpcService(gui::PostprocessWidget *widget,
                                   Transport *transport, QObject *parent = nullptr);
    ~PostprocessRpcService() override;

    /// Send the `ready` notification (call after the window is shown).
    void NotifyReady(const QString &title);

    /// Send a `status_changed` notification.
    void NotifyStatus(const QString &text);

    /// Direct access for hosts that need raw calls (rarely needed).
    JsonRpcPeer *peer() const
    {
        return peer_;
    }

  private:
    void RegisterMethods();

    gui::PostprocessWidget *widget_ = nullptr;
    Transport *transport_ = nullptr;
    JsonRpcPeer *peer_ = nullptr;
};

} // namespace rpc
} // namespace xresults
