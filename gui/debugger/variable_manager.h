#pragma once
#include <QObject>
#include <QString>
#include <QSet>

namespace xequation
{
namespace gui
{

class VariableManager;

class Variable
{
  public:
    Variable(const QString &name, const QString &value, const QString &type);
    ~Variable();

    void AddChild(Variable *child);
    void AddChildren(const QList<Variable *> &children);
    void RemoveChild(Variable *child);
    void RemoveChildren(const QList<Variable *> &children);
    void ClearChildren();

    Variable *GetChildAt(int index) const;
    int IndexOfChild(Variable *child) const;

    void SetValue(const QString &value);
    void SetType(const QString &type);

    const QString &name() const
    {
        return name_;
    }

    const QString &value() const
    {
        return value_;
    }

    const QString &type() const
    {
        return type_;
    }

    Variable *parent() const
    {
        return parent_;
    }

    int ChildCount() const
    {
        return child_list_.size();
    }
    
    QList<Variable *> children() const
    {
        return child_list_;
    }

    VariableManager* manager()
    {
        return manager_;
    }

  private:
    friend class VariableManager;
    QString name_;
    QString value_;
    QString type_;
    QList<Variable*> child_list_;
    Variable *parent_;
    VariableManager* manager_;
};

class VariableManager : public QObject
{
    Q_OBJECT
  public:
    VariableManager(QObject* parent = nullptr);

    ~VariableManager();

    Variable *CreateVariable(const QString &name, const QString &value, const QString &type);
    void RemoveVariable(Variable* variable);
    void Clear();

    bool IsContain(Variable* variable) const;
    void BeginUpdate();
    void EndUpdate();
    void SetVariableValue(Variable* variable, const QString &value);
    void SetVariableType(Variable* variable, const QString &type);
    
    void AddVariableChild(Variable* parent, Variable* child);
    void RemoveVariableChild(Variable* parent, Variable* child);
    void AddVariableChildren(Variable* parent, const QList<Variable*>& children);
    void RemoveVariableChildren(Variable* parent, const QList<Variable*>& children);

    int Count() const
    {
        return variable_set_.size();
    }

    const QSet<Variable*>& variable_set() const
    {
        return variable_set_;
    }

  signals:
    void VariableChanged(Variable* variable);
    void VariablesChanged(const QList<Variable*>& variables);
    void VariableChildInserted(Variable* parent, Variable* child);
    void VariableChildRemoved(Variable* parent, Variable* child);
    void VariableChildrenInserted(Variable* parent, const QList<Variable*>& children);
    void VariableChildrenRemoved(Variable* parent, const QList<Variable*>& children);

  private:
    QSet<Variable*> variable_set_;
    bool updating_ = false;
    QList<Variable*> updated_variables_;
};

} // namespace gui
} // namespace xequation
