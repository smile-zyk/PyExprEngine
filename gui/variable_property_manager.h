#pragma once

#include <QtProperty>

// Modified from QtStringPropertyManager
// QtProperty with Value and Type

namespace xequation
{
namespace gui
{
class VariablePropertyManagerPrivate;

class VariablePropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    VariablePropertyManager(QObject *parent = 0);
    ~VariablePropertyManager();

    QString value(const QtProperty *property) const;
    QString type(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, const QString &val);
    void setType(QtProperty *property, const QString &type);
Q_SIGNALS:
    void valueChanged(QtProperty *property, const QString &val);
    void typeChanged(QtProperty *property, const QString &type);
protected:
    QString valueText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);
private:
    QScopedPointer<VariablePropertyManagerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(VariablePropertyManager)
    Q_DISABLE_COPY_MOVE(VariablePropertyManager)
};
} // namespace gui
} // namespace xequation