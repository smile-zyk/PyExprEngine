#pragma once

#include <QtProperty>

// Modified from QtStringPropertyManager
// QtProperty with Value and Type

namespace xequation
{
namespace gui
{
using VariableProperty = QtProperty;

class VariablePropertyManagerPrivate;

class VariablePropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    VariablePropertyManager(QObject *parent = 0);
    ~VariablePropertyManager();

    QString value(const VariableProperty *property) const;
    QString type(const VariableProperty *property) const;

public Q_SLOTS:
    void setValue(VariableProperty *property, const QString &val);
    void setType(VariableProperty *property, const QString &type);
Q_SIGNALS:
    void valueChanged(VariableProperty *property, const QString &val);
    void typeChanged(VariableProperty *property, const QString &type);
protected:
    QString valueText(const VariableProperty *property) const;
    virtual void initializeProperty(VariableProperty *property);
    virtual void uninitializeProperty(VariableProperty *property);
private:
    QScopedPointer<VariablePropertyManagerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(VariablePropertyManager)
    Q_DISABLE_COPY_MOVE(VariablePropertyManager)
};
} // namespace gui
} // namespace xequation