#include "postprocess_rpc_service.h"

#include <QCoreApplication>

#include "postprocess_widget.h"
#include "json_rpc_peer.h"
#include "methods/method_modules.h"
#include "transport.h"

namespace xresults
{
namespace rpc
{

PostprocessRpcService::PostprocessRpcService(gui::PostprocessWidget *widget,
                                             Transport *transport, QObject *parent)
    : QObject(parent), widget_(widget), transport_(transport)
{
    transport_->setParent(this);
    peer_ = new JsonRpcPeer(transport_, this);

    RegisterMethods();

    // The host closed the connection (or exited) -> quit, so a postprocess
    // window is never left orphaned.
    connect(transport_, &Transport::Disconnected, this, [this]() {
        if (widget_)
        {
            widget_->close();
        }
        QCoreApplication::quit();
    });
}

PostprocessRpcService::~PostprocessRpcService() = default;

void PostprocessRpcService::NotifyReady(const QString &title)
{
    QJsonObject params;
    params["title"] = title;
    params["pid"] = static_cast<double>(QCoreApplication::applicationPid());
    peer_->Notify(QStringLiteral("ready"), params);
}

void PostprocessRpcService::NotifyStatus(const QString &text)
{
    QJsonObject params;
    params["text"] = text;
    peer_->Notify(QStringLiteral("status_changed"), params);
}

void PostprocessRpcService::RegisterMethods()
{
    // One line per module -- adding a module is adding a line here.
    methods::RegisterSystemMethods(peer_, widget_);
    methods::RegisterWindowMethods(peer_, widget_);
    methods::RegisterEquationMethods(peer_, widget_);
    methods::RegisterDatasetMethods(peer_, widget_);
    methods::RegisterProjectMethods(peer_, widget_);
}

} // namespace rpc
} // namespace xresults
