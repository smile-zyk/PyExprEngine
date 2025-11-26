#include "variable_property_manager.h"
#include <QMap>

namespace xequation
{
namespace gui
{
class VariablePropertyManagerPrivate
{
    VariablePropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(VariablePropertyManager)
public:

    struct Data
    {
        QString val;
        QString type;
    };

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    QMap<const QtProperty *, Data> m_values;
};

VariablePropertyManager::VariablePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent), d_ptr(new VariablePropertyManagerPrivate)
{
    d_ptr->q_ptr = this;
}

VariablePropertyManager::~VariablePropertyManager()
{
    clear();
}

QString VariablePropertyManager::value(const QtProperty *property) const
{
    const auto it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().val;
}

QString VariablePropertyManager::type(const QtProperty *property) const
{
    const auto it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().type;
}

QString VariablePropertyManager::valueText(const QtProperty *property) const
{
    return value(property);
}

void VariablePropertyManager::setValue(QtProperty *property, const QString &val)
{
    const VariablePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VariablePropertyManagerPrivate::Data data = it.value();

    if (data.val == val)
        return;

    data.val = val;

    it.value() = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
}

void VariablePropertyManager::setType(QtProperty *property, const QString &type)
{
    const VariablePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    VariablePropertyManagerPrivate::Data data = it.value() ;

    if (data.type == type)
        return;

    data.type = type;

    it.value() = data;

    emit propertyChanged(property);
    emit typeChanged(property, data.type);
}

void VariablePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = VariablePropertyManagerPrivate::Data();
}

void VariablePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

} // namespace gui
} // namespace xequation