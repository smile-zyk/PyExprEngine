// =========================================================================
// demo -- the host application.
//
// It does no post-processing itself: it launches several `postprocess` child
// processes and drives each of them over JSON-RPC 2.0.  See LauncherWidget
// for the UI and host/postprocess_manager.h for the library it uses.
// =========================================================================

#include <QApplication>

#include "launcher_widget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    xresults::gui::LauncherWidget widget;
    widget.show();

    return app.exec();
}
