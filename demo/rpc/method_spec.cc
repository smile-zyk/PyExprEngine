#include "method_spec.h"

namespace xresults
{
namespace rpc
{
namespace
{
const char *JsonTypeName(ParamType type)
{
    switch (type)
    {
    case ParamType::kString:
        return "string";
    case ParamType::kBool:
        return "boolean";
    case ParamType::kInt:
        return "integer";
    case ParamType::kNumber:
        return "number";
    case ParamType::kObject:
        return "object";
    case ParamType::kArray:
        return "array";
    }
    return "string";
}
} // namespace

QJsonObject ParamSpec::ToJsonSchema() const
{
    QJsonObject schema;
    schema["type"] = QString::fromLatin1(JsonTypeName(type));
    if (!description.isEmpty())
    {
        schema["description"] = description;
    }
    if (!default_value.isNull() && !default_value.isUndefined())
    {
        schema["default"] = default_value;
    }
    return schema;
}

QJsonObject MethodSpec::ToInputSchema() const
{
    QJsonObject properties;
    QJsonArray required;
    for (const ParamSpec &p : params)
    {
        properties[p.name] = p.ToJsonSchema();
        if (p.required)
        {
            required.append(p.name);
        }
    }

    QJsonObject schema;
    schema["type"] = QStringLiteral("object");
    schema["properties"] = properties;
    if (!required.isEmpty())
    {
        schema["required"] = required;
    }
    return schema;
}

} // namespace rpc
} // namespace xresults
