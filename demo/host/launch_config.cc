#include "launch_config.h"

#include <QFile>
#include <QJsonDocument>

#include "../rpc/json_rpc_message.h"

namespace xresults
{
namespace host
{

QJsonObject LaunchConfig::ToJson() const
{
    QJsonObject o;
    o["version"] = 1;
    if (!title.isEmpty())
    {
        o["title"] = title;
    }
    if (!datasets.isEmpty())
    {
        o["datasets"] = datasets;
    }
    if (!default_dataset.isEmpty())
    {
        o["default_dataset"] = default_dataset;
    }
    if (!equations.isEmpty())
    {
        o["equations"] = equations;
    }
    if (!expressions.isEmpty())
    {
        o["expressions"] = expressions;
    }
    return o;
}

LaunchConfig LaunchConfig::FromJson(const QJsonObject &o)
{
    LaunchConfig cfg;
    cfg.title = rpc::GetString(o, QStringLiteral("title"));
    cfg.datasets = rpc::GetArray(o, QStringLiteral("datasets"));
    cfg.default_dataset = rpc::GetString(o, QStringLiteral("default_dataset"));
    cfg.equations = rpc::GetArray(o, QStringLiteral("equations"));
    cfg.expressions = rpc::GetArray(o, QStringLiteral("expressions"));
    return cfg;
}

LaunchConfig LaunchConfig::FromFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
        {
            *error =
                QStringLiteral("cannot open %1: %2").arg(path, file.errorString());
        }
        return LaunchConfig();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
    {
        if (error)
        {
            *error = QStringLiteral("%1 is not a JSON object").arg(path);
        }
        return LaunchConfig();
    }
    return FromJson(doc.object());
}

} // namespace host
} // namespace xresults
