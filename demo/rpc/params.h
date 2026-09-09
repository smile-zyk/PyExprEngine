#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace xresults
{
namespace rpc
{

class JsonRpcError;

// =========================================================================
// JsonTraits<T> -- how to read a T out of a JSON object member.  Specialize
// this to teach Params a new type:
//
//   template <> struct JsonTraits<MyType> {
//       static MyType Get(const QJsonObject &o, const QString &key, bool *ok);
//       static const char *TypeName();
//   };
// =========================================================================

template <typename T>
struct JsonTraits;

template <>
struct JsonTraits<QString>
{
    static QString Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isString();
        return *ok ? v.toString() : QString();
    }
    static const char *TypeName()
    {
        return "string";
    }
};

template <>
struct JsonTraits<bool>
{
    static bool Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isBool();
        return *ok && v.toBool();
    }
    static const char *TypeName()
    {
        return "bool";
    }
};

template <>
struct JsonTraits<int>
{
    static int Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isDouble();
        return *ok ? static_cast<int>(v.toDouble()) : 0;
    }
    static const char *TypeName()
    {
        return "int";
    }
};

template <>
struct JsonTraits<double>
{
    static double Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isDouble();
        return *ok ? v.toDouble() : 0.0;
    }
    static const char *TypeName()
    {
        return "number";
    }
};

template <>
struct JsonTraits<QJsonObject>
{
    static QJsonObject Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isObject();
        return *ok ? v.toObject() : QJsonObject();
    }
    static const char *TypeName()
    {
        return "object";
    }
};

template <>
struct JsonTraits<QJsonArray>
{
    static QJsonArray Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        *ok = v.isArray();
        return *ok ? v.toArray() : QJsonArray();
    }
    static const char *TypeName()
    {
        return "array";
    }
};

template <>
struct JsonTraits<QStringList>
{
    static QStringList Get(const QJsonObject &o, const QString &key, bool *ok)
    {
        const QJsonValue v = o.value(key);
        if (!v.isArray())
        {
            *ok = false;
            return QStringList();
        }
        QStringList out;
        for (const QJsonValue &item : v.toArray())
        {
            if (!item.isString())
            {
                *ok = false;
                return QStringList();
            }
            out.append(item.toString());
        }
        *ok = true;
        return out;
    }
    static const char *TypeName()
    {
        return "array of string";
    }
};

// =========================================================================
// Params -- typed access to a handler's `params` object, removing per-method
// validation boilerplate:
//
//   const QString name = p.Require<QString>("name");      // throws if missing
//   const bool    flag = p.Optional<bool>("flag", false); // default if missing
//
// Require<T>() throws JsonRpcError(kInvalidParams) when the member is absent
// or of the wrong type, so handlers need no validation code.
// =========================================================================

class Params
{
  public:
    explicit Params(const QJsonObject &o) : o_(o) {}

    const QJsonObject &object() const
    {
        return o_;
    }

    /// Required member.  Throws JsonRpcError(kInvalidParams) when absent or
    /// of the wrong type.
    template <typename T>
    T Require(const QString &key) const;

    /// Optional member: @p def when absent or of the wrong type.
    template <typename T>
    T Optional(const QString &key, const T &def = T()) const;

    bool Has(const QString &key) const
    {
        return o_.contains(key);
    }

  private:
    QJsonObject o_;
};

// Throwing from a header would need JsonRpcError to be complete; it is
// declared in json_rpc_peer.h, which includes this header.  Define the
// templates after that point via a small helper declared here and defined in
// json_rpc_peer.cc.
[[noreturn]] void ThrowInvalidParams(const QString &key, const char *type_name);

template <typename T>
T Params::Require(const QString &key) const
{
    bool ok = false;
    const T value = JsonTraits<T>::Get(o_, key, &ok);
    if (!ok)
    {
        ThrowInvalidParams(key, JsonTraits<T>::TypeName());
    }
    return value;
}

template <typename T>
T Params::Optional(const QString &key, const T &def) const
{
    bool ok = false;
    const T value = JsonTraits<T>::Get(o_, key, &ok);
    return ok ? value : def;
}

} // namespace rpc
} // namespace xresults
