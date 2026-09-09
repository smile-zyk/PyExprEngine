#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <functional>

#include "params.h"

namespace xresults
{
namespace rpc
{

// =========================================================================
// MethodSpec -- a method plus its metadata.
//
// Required (not optional): MCP's `tools/list` must return a JSON Schema for
// every tool, and that schema can only come from the parameter descriptions
// below.  It also drives `system.list_methods` grouping and lets a host build
// a command UI without hard-coding anything.
// =========================================================================

enum class ParamType
{
    kString,
    kBool,
    kInt,
    kNumber,
    kObject,
    kArray,
};

struct ParamSpec
{
    QString name;
    ParamType type = ParamType::kString;
    bool required = false;
    QJsonValue default_value;
    QString description;

    // C++11: a class with default member initializers is NOT an aggregate, so
    // brace initialization needs an explicit constructor.
    ParamSpec() = default;
    ParamSpec(const QString &name_, ParamType type_, bool required_,
              const QJsonValue &default_value_, const QString &description_)
        : name(name_), type(type_), required(required_),
          default_value(default_value_), description(description_)
    {
    }

    /// JSON Schema fragment for this parameter (MCP tools/list).
    QJsonObject ToJsonSchema() const;
};

struct MethodSpec
{
    QString name;                    // e.g. "dataset.add"
    QString description;
    QVector<ParamSpec> params;
    std::function<QJsonValue(const Params &)> handler;

    /// JSON Schema of the whole `params` object (MCP inputSchema).
    QJsonObject ToInputSchema() const;

    /// The `module` part of `module.verb`, or the whole name when unprefixed.
    QString Module() const
    {
        const int dot = name.indexOf('.');
        return dot > 0 ? name.left(dot) : name;
    }
};

} // namespace rpc
} // namespace xresults
