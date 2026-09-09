#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace xresults
{
namespace host
{

// =========================================================================
// LaunchConfig -- the startup parameters of one postprocess instance.
//
// It is a superset of `demo_project.json` (same datasets / default_dataset /
// equations / expressions keys) plus a couple of window options, so an
// existing project file can be handed over verbatim.
//
// The config is written to a temp JSON file and passed as `--config <file>`;
// the transport's own arguments travel separately on the command line.
// =========================================================================

struct LaunchConfig
{
    /// Window title (also the display name in the host's list).
    /// Empty -> the child picks a default.
    QString title;

    /// Datasets: each entry is {name, format, path}.
    QJsonArray datasets;

    /// Name of the dataset that becomes the REL default (optional).
    QString default_dataset;

    /// Equations: each entry is {name, content, tag}.
    QJsonArray equations;

    /// Watch expressions: each entry is {content, tag}.
    QJsonArray expressions;

    /// Working directory of the child.  Empty -> the directory of the
    /// executable, so relative dataset paths resolve as in standalone mode.
    QString working_directory;

    QJsonObject ToJson() const;

    /// Parse from a JSON object (the same schema ToJson writes; the
    /// project-file keys are read too, so a demo_project.json parses fine).
    static LaunchConfig FromJson(const QJsonObject &o);

    /// Parse from a JSON file.  Returns a default config (and fills @p error)
    /// when the file is missing or not a JSON object.
    static LaunchConfig FromFile(const QString &path, QString *error = nullptr);
};

} // namespace host
} // namespace xresults
