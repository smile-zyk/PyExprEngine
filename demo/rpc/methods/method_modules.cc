#include "method_modules.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <boost/uuid/uuid_io.hpp>

#include "../../postprocess_widget.h"
#include "../json_rpc_peer.h"
#include "../method_spec.h"
#include "core/equation_manager.h"

namespace xresults
{
namespace rpc
{
namespace methods
{
namespace
{
/// ObjectId -> canonical string form (for the `id` result field).
QString IdToString(const xequation::ObjectId &id)
{
    return QString::fromStdString(boost::uuids::to_string(id));
}

/// {"ok":true} -- the common success shape.
QJsonObject Ok()
{
    QJsonObject o;
    o["ok"] = true;
    return o;
}
} // namespace

// =========================================================================
// window.*
// =========================================================================

void RegisterWindowMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget)
{
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("window.raise"),
        QStringLiteral("Bring the postprocess window to the front"),
        {},
        [widget](const Params &) {
            widget->RaiseWindow();
            return QJsonValue(Ok());
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("window.set_status"),
        QStringLiteral("Set the text shown in the window's status bar"),
        QVector<ParamSpec>{ParamSpec{QStringLiteral("text"), ParamType::kString,
                                     true, QJsonValue(),
                                     QStringLiteral("status text")}},
        [widget](const Params &p) {
            widget->SetStatusText(p.Require<QString>(QStringLiteral("text")));
            return QJsonValue(Ok());
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("window.set_title"),
        QStringLiteral("Set the window title"),
        QVector<ParamSpec>{ParamSpec{QStringLiteral("title"), ParamType::kString,
                                     true, QJsonValue(),
                                     QStringLiteral("new title")}},
        [widget](const Params &p) {
            const QString title = p.Require<QString>(QStringLiteral("title"));
            widget->setWindowTitle(title);
            return QJsonValue(Ok());
        }});
}

// =========================================================================
// equation.* / expression.*
// =========================================================================

void RegisterEquationMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget)
{
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("equation.add"),
        QStringLiteral("Add an equation, or redefine it when the name is taken"),
        QVector<ParamSpec>{
            ParamSpec{QStringLiteral("name"), ParamType::kString, true,
                      QJsonValue(),
                      QStringLiteral("equation name (identifier)")},
            ParamSpec{QStringLiteral("content"), ParamType::kString, true,
                      QJsonValue(),
                      QStringLiteral("expression, e.g. LNA.amplifier.HB1.HB.Gain")},
            ParamSpec{QStringLiteral("tag"), ParamType::kString, false,
                      QJsonValue(QStringLiteral("Equation")),
                      QStringLiteral("display tag")},
            ParamSpec{QStringLiteral("redefine"), ParamType::kBool, false,
                      QJsonValue(false),
                      QStringLiteral("update in place when the name exists")},
        },
        [widget](const Params &p) {
            const QString name = p.Require<QString>(QStringLiteral("name"));
            const QString content = p.Require<QString>(QStringLiteral("content"));
            const QString tag = p.Optional<QString>(QStringLiteral("tag"));
            const bool redefine = p.Optional<bool>(QStringLiteral("redefine"), false);

            const xequation::ObjectId id =
                widget->AddEquation(name, content, tag, redefine);

            QJsonObject o = Ok();
            o["id"] = IdToString(id);
            o["name"] = name;
            return QJsonValue(o);
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("expression.add"),
        QStringLiteral("Register a watch expression and open a tab for it"),
        QVector<ParamSpec>{
            ParamSpec{QStringLiteral("content"), ParamType::kString, true,
                      QJsonValue(),
                      QStringLiteral("expression, e.g. LNA.amplifier.HB1.HB.Pout")},
            ParamSpec{QStringLiteral("tag"), ParamType::kString, false,
                      QJsonValue(QStringLiteral("Watch")),
                      QStringLiteral("display tag")},
        },
        [widget](const Params &p) {
            const QString content = p.Require<QString>(QStringLiteral("content"));
            const QString tag = p.Optional<QString>(QStringLiteral("tag"));

            const xequation::ObjectId id = widget->AddExpression(content, tag);

            QJsonObject o = Ok();
            o["id"] = IdToString(id);
            return QJsonValue(o);
        }});
}

// =========================================================================
// dataset.*
// =========================================================================

void RegisterDatasetMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget)
{
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("dataset.add"),
        QStringLiteral("Load a dataset file into the REL environment"),
        QVector<ParamSpec>{
            ParamSpec{QStringLiteral("name"), ParamType::kString, true,
                      QJsonValue(), QStringLiteral("dataset name")},
            ParamSpec{QStringLiteral("path"), ParamType::kString, true,
                      QJsonValue(), QStringLiteral("dataset file path")},
            ParamSpec{QStringLiteral("format"), ParamType::kString, false,
                      QJsonValue(QStringLiteral("hdf5")),
                      QStringLiteral("hdf5 | touchstone")},
            ParamSpec{QStringLiteral("make_default"), ParamType::kBool, false,
                      QJsonValue(false),
                      QStringLiteral("also make it the default dataset")},
        },
        [widget](const Params &p) {
            const QString name = p.Require<QString>(QStringLiteral("name"));
            const QString path = p.Require<QString>(QStringLiteral("path"));
            const QString format =
                p.Optional<QString>(QStringLiteral("format"), QStringLiteral("hdf5"));
            const bool make_default =
                p.Optional<bool>(QStringLiteral("make_default"), false);

            widget->AddDataset(name, format, path, make_default);

            QJsonObject o = Ok();
            o["default_dataset"] = widget->DefaultDatasetName();
            return QJsonValue(o);
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("dataset.remove"),
        QStringLiteral("Remove a dataset from the REL environment"),
        QVector<ParamSpec>{ParamSpec{QStringLiteral("name"), ParamType::kString,
                                     true, QJsonValue(),
                                     QStringLiteral("dataset name")}},
        [widget](const Params &p) {
            widget->RemoveDataset(p.Require<QString>(QStringLiteral("name")));
            return QJsonValue(Ok());
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("dataset.set_default"),
        QStringLiteral("Switch the REL default dataset and recompute"),
        QVector<ParamSpec>{ParamSpec{QStringLiteral("name"), ParamType::kString,
                                     true, QJsonValue(),
                                     QStringLiteral("dataset name")}},
        [widget](const Params &p) {
            const QString name = p.Require<QString>(QStringLiteral("name"));
            widget->SetDefaultDataset(name);
            QJsonObject o = Ok();
            o["default_dataset"] = name;
            return QJsonValue(o);
        }});
}

// =========================================================================
// project.*
// =========================================================================

void RegisterProjectMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget)
{
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("project.load"),
        QStringLiteral("Load a project file, replacing the current one"),
        QVector<ParamSpec>{ParamSpec{QStringLiteral("path"), ParamType::kString,
                                     true, QJsonValue(),
                                     QStringLiteral("project JSON path")}},
        [widget](const Params &p) {
            const QString path = p.Require<QString>(QStringLiteral("path"));
            widget->LoadProjectOrThrow(path);
            QJsonObject o = Ok();
            o["project"] = path;
            return QJsonValue(o);
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("project.get_state"),
        QStringLiteral("Query the current datasets / equations / window title"),
        {},
        [widget](const Params &) {
            QJsonArray datasets;
            for (const std::string &name : widget->DatasetNames())
            {
                datasets.append(QString::fromStdString(name));
            }
            QJsonArray equations;
            for (const std::string &name :
                 xequation::EquationManager::GetInstance().GetEquationNames())
            {
                equations.append(QString::fromStdString(name));
            }

            QJsonObject o = Ok();
            o["title"] = widget->windowTitle();
            o["datasets"] = datasets;
            o["default_dataset"] = widget->DefaultDatasetName();
            o["equations"] = equations;
            return QJsonValue(o);
        }});
}

// =========================================================================
// system.*
// =========================================================================

void RegisterSystemMethods(JsonRpcPeer *peer, gui::PostprocessWidget *widget)
{
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("system.ping"), QStringLiteral("Liveness check"), {},
        [](const Params &) {
            QJsonObject o;
            o["pong"] = true;
            o["pid"] = static_cast<double>(QCoreApplication::applicationPid());
            return QJsonValue(o);
        }});

    peer->RegisterMethod(MethodSpec{
        QStringLiteral("system.list_methods"),
        QStringLiteral("List every registered method, grouped by module"), {},
        [peer](const Params &) {
            QJsonArray methods;
            for (const QString &name : peer->MethodNames())
            {
                methods.append(name);
            }
            QJsonObject o;
            o["methods"] = methods;
            o["groups"] = peer->MethodGroups();
            return QJsonValue(o);
        }});

    // Works as both a request and a notification: the host can fire it and
    // forget, or wait for the ack.
    peer->RegisterMethod(MethodSpec{
        QStringLiteral("system.shutdown"),
        QStringLiteral("Close the window and quit the process"), {},
        [widget](const Params &) {
            if (widget)
            {
                widget->close();
            }
            QCoreApplication::quit();
            return QJsonValue(Ok());
        }});
}

} // namespace methods
} // namespace rpc
} // namespace xresults
