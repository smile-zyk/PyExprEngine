#pragma once

namespace xresults
{
namespace gui
{
class PostprocessWidget;
}

namespace rpc
{
class JsonRpcPeer;

namespace methods
{

// =========================================================================
// One registration function per method module; each registers the methods of
// one `module.` prefix.  Adding a module is: new methods/xxx_methods.{h,cc}
// with RegisterXxxMethods(), plus one call in PostprocessRpcService.
//
// @p widget must outlive @p peer (handlers capture it).
// =========================================================================

/// window.raise / window.set_status / window.set_title
void RegisterWindowMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget);

/// equation.add / expression.add
void RegisterEquationMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget);

/// dataset.add / dataset.remove / dataset.set_default
void RegisterDatasetMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget);

/// project.load / project.get_state
void RegisterProjectMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget);

/// system.ping / system.list_methods / system.shutdown
void RegisterSystemMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget);

} // namespace methods
} // namespace rpc
} // namespace xresults
