#include "variable_manager.h"

namespace xequation
{
namespace gui
{
Variable::Variable(const QString &name, const QString &value, const QString &type)
    : name_(name), value_(value), type_(type), parent_(nullptr)
{
}

Variable::~Variable()
{
    for (auto *child : child_list_)
    {
        if (child)
            child->parent_ = nullptr;
    }
    child_list_.clear();
    parent_ = nullptr;
}

void Variable::AddChild(Variable *child)
{
    manager_->AddVariableChild(this, child);
}

void Variable::AddChildren(const QList<Variable *> &children)
{
    manager_->AddVariableChildren(this, children);
}

void Variable::RemoveChild(Variable *child)
{
    manager_->RemoveVariableChild(this, child);
}

void Variable::RemoveChildren(const QList<Variable *> &children)
{
    manager_->RemoveVariableChildren(this, children);
}

Variable *Variable::GetChildAt(int index) const
{
    if (index < 0 || index >= child_list_.size())
        return nullptr;
    return child_list_.at(index);
}

void Variable::ClearChildren()
{
    manager_->RemoveVariableChildren(this, child_list_);
}

void Variable::SetValue(const QString &value)
{
    manager_->SetVariableValue(this, value);
}

void Variable::SetType(const QString &type)
{
    manager_->SetVariableType(this, type);
}

int Variable::IndexOfChild(Variable *child) const
{
    return child_list_.indexOf(child);
}

VariableManager::VariableManager(QObject *parent) : QObject(parent) {}

VariableManager::~VariableManager()
{
    Clear();
}

Variable *VariableManager::CreateVariable(const QString &name, const QString &value, const QString &type)
{
    Variable *raw = new Variable(name, value, type);
    raw->manager_ = this;
    variable_set_.insert(raw);
    return raw;
}

void VariableManager::RemoveVariable(Variable *variable)
{
    if (!variable || !variable_set_.contains(variable))
        return;
    variable_set_.remove(variable);
    delete variable;
}

void VariableManager::Clear()
{
    for (Variable *var : variable_set_)
    {
        delete var;
    }
    variable_set_.clear();
}

bool VariableManager::IsContain(Variable* variable) const
{
    return variable_set_.contains(variable);
}

void VariableManager::BeginUpdate()
{
    updating_ = true;
}

void VariableManager::EndUpdate()
{
    updating_ = false;
    emit VariablesChanged(updated_variables_);
    updated_variables_.clear();
}

void VariableManager::SetVariableValue(Variable* variable, const QString &value)
{
    variable->value_ = value;
    if(updating_ == false)
    {
        emit VariableChanged(variable);
    }
    else 
    {
        updated_variables_.append(variable);
    }
}

void VariableManager::SetVariableType(Variable* variable, const QString &type)
{
    variable->type_ = type;
    if(updating_ == false)
    {
        emit VariableChanged(variable);
    }
    else 
    {
        updated_variables_.append(variable);
    }
}
    
void VariableManager::AddVariableChild(Variable* parent, Variable* child)
{
    if( !parent || !child || parent == child)
        return;
    parent->child_list_.append(child);
    child->parent_ = parent;
    emit VariableChildInserted(parent, child);
}

void VariableManager::RemoveVariableChild(Variable* parent, Variable* child)
{
    if (!parent || !child)
        return;
    parent->child_list_.removeOne(child);
    child->parent_ = nullptr;
    emit VariableChildRemoved(parent, child);
}

void VariableManager::AddVariableChildren(Variable* parent, const QList<Variable*>& children)
{
    for (Variable *child : children)
    {
        if(!parent || !child || parent == child)
            continue;
        parent->child_list_.append(child);
        child->parent_ = parent;
    }
    emit VariableChildrenInserted(parent, children);
}

void VariableManager::RemoveVariableChildren(Variable* parent, const QList<Variable*>& children)
{
    for (Variable *child : children)
    {
        if (!parent || !child)
            continue;
        parent->child_list_.removeOne(child);
        child->parent_ = nullptr;
    }
    emit VariableChildrenRemoved(parent, children);
}

} // namespace gui
} // namespace xequation