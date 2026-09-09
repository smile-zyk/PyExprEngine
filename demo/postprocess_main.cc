// =========================================================================
// postprocess -- the post-processing application.
//
//   postprocess                                           standalone
//   postprocess --rpc --channel <name> [--config <file>]  RPC child
//
// Startup parameters come only from --config (datasets / equations /
// expressions).  In RPC mode the process quits when the host closes the
// transport or sends `system.shutdown`, so a host crash never leaves an
// orphan window behind.
// =========================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTimer>

#include "postprocess_widget.h"
#include "rpc/local_socket_transport.h"
#include "rpc/postprocess_rpc_service.h"
#include "rpc/transport.h"

#ifdef DEMO_HAS_PYTHON
#include <exception>
#include "environment.h"     // rel::Environment::CleanupPythonState
#include "python_manager.h"  // python_manager::PyEnvManager
#endif

namespace
{
/// Read the --config JSON file.  Returns an empty object (and fills @p error)
/// when the file is missing or not a JSON object.
QJsonObject ReadConfigFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
        {
            *error = QStringLiteral("cannot open config %1: %2")
                         .arg(path, file.errorString());
        }
        return QJsonObject();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
    {
        if (error)
        {
            *error = QStringLiteral("config %1 is not a JSON object").arg(path);
        }
        return QJsonObject();
    }
    return doc.object();
}
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef DEMO_HAS_PYTHON
    // REL 嵌入式 Python：解释器生命周期归宿主所有（rel 只执行插件，从不
    // 自行创建/销毁解释器，见 python_loader.cc）。失败不致命：只是
    // python_plugins 不可用，dataset 加载不受影响。
    try
    {
        python_manager::PyEnvManager::SetDefaultPyEnvConfig();
        python_manager::PyEnvManager::InitializePyEnv();
    }
    catch (const std::exception &e)
    {
        qWarning("Failed to initialize embedded Python: %s", e.what());
    }
#endif

    // ---- command line ---------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription("XEquation post-processor");
    parser.addHelpOption();
    QCommandLineOption rpc_option(QStringList() << "rpc",
                                  "Run as a JSON-RPC 2.0 child process.");
    QCommandLineOption channel_option(
        QStringList() << "channel",
        "Name of the host's QLocalServer to connect to (with --rpc).", "name");
    QCommandLineOption config_option(QStringList() << "config",
                                     "Startup config JSON file.", "file");
    parser.addOption(rpc_option);
    parser.addOption(channel_option);
    parser.addOption(config_option);
    parser.process(app);

    const bool rpc_mode = parser.isSet(rpc_option);

    // The protocol runs on a local socket, so stdout is free -- but keeping
    // diagnostics on stderr is still the right default: it keeps the console
    // clean and matches what the host reads on a separate channel.
    if (rpc_mode)
    {
        xresults::rpc::LocalSocketTransport::RedirectQtLoggingToStderr();
    }

    xresults::gui::PostprocessWidget widget;

    // ---- startup parameters ---------------------------------------------
    QString config_error;
    QJsonObject config;
    if (parser.isSet(config_option))
    {
        config = ReadConfigFile(parser.value(config_option), &config_error);
    }

    QString title = config.value(QStringLiteral("title")).toString();
    if (title.isEmpty() && !rpc_mode)
    {
        title = QStringLiteral("XEquation Postprocess");
    }
    if (!title.isEmpty())
    {
        widget.setWindowTitle(title);
    }

    // ---- RPC service (created before show() so `ready` is the first thing
    // ---- the host sees) --------------------------------------------------
    xresults::rpc::PostprocessRpcService *service = nullptr;
    if (rpc_mode)
    {
        const QString channel = parser.value(channel_option);
        if (channel.isEmpty())
        {
            qWarning("postprocess: --rpc requires --channel <name>");
            return 1;
        }
        xresults::rpc::Transport *transport =
            new xresults::rpc::LocalSocketTransport(
                xresults::rpc::Transport::Role::kChild, channel, &widget);

        QString start_error;
        if (!transport->Start(&start_error))
        {
            qWarning("postprocess: cannot start transport: %s",
                     qPrintable(start_error));
            return 1;
        }
        service = new xresults::rpc::PostprocessRpcService(&widget, transport,
                                                           &widget);
    }

    widget.show();

    // Startup work is deferred to the event loop: loading datasets reads HDF5
    // and runs Python plugins, which can take seconds.  Doing it before
    // app.exec() would block the GUI thread while the window is already
    // visible but unpainted -- it would look hung.
    QTimer::singleShot(0, [&]() {
        if (!config_error.isEmpty())
        {
            widget.SetStatusText(config_error);
        }

        QStringList startup_errors;
        widget.ApplyStartupConfig(config, &startup_errors);
        if (!startup_errors.isEmpty())
        {
            widget.SetStatusText(startup_errors.join(QStringLiteral("; ")));
        }

        // Announce readiness only after the startup work, so a command sent on
        // `ready` never races it.
        if (service)
        {
            service->NotifyReady(widget.windowTitle());
        }
    });

    const int result = app.exec();

#ifdef DEMO_HAS_PYTHON
    // 逆序清理：先释放 pybind11 持有的回调（解释器必须还活着），再销毁解释器。
    if (python_manager::PyEnvManager::IsInitialized())
    {
        rel::Environment::CleanupPythonState();
        python_manager::PyEnvManager::ShutdownPyEnv();
    }
#endif

    return result;
}
