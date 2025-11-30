#include "variable_data_manager.h"

namespace xequation
{
namespace gui
{
VariableModelData::VariableModelData(const QString &name, const QString &value, const QString &type)
    : name_(name), value_(value), type_(type), parent_(nullptr)
{
}

VariableModelData::~VariableModelData()
{
    for (auto *child : child_list_)
    {
        if (child)
            child->parent_ = nullptr;
    }
    child_list_.clear();
    child_map_.clear();
    parent_ = nullptr;
}

void VariableModelData::AddChild(VariableModelData *child)
{
    child_list_.append(child);
    child_map_.insert(child->name_, child);
    child->parent_ = this;
}

void VariableModelData::AddChildren(const QList<VariableModelData *> &children)
{
    for (VariableModelData *child : children)
    {
        if (child)
        {
            child_list_.append(child);
            child_map_.insert(child->name_, child);
            child->parent_ = this;
        }
    }
}

void VariableModelData::RemoveChild(const QString &name)
{
    auto it = child_map_.find(name);
    if (it == child_map_.end())
        return;
    VariableModelData *c = it.value();
    child_map_.erase(it);
    child_list_.removeOne(*it);
    if (c)
        c->parent_ = nullptr;
}

void VariableModelData::RemoveChild(VariableModelData *child)
{
    if (!child)
        return;
    child_map_.remove(child->name());
    child_list_.removeOne(child);
    child->parent_ = nullptr;
}

VariableModelData *VariableModelData::GetChild(const QString &name) const
{
    auto it = child_map_.find(name);
    return (it == child_map_.end()) ? nullptr : it.value();
}

VariableModelData *VariableModelData::GetChildAt(int index) const
{
    if (index < 0 || index >= child_list_.size())
        return nullptr;
    return child_list_.at(index);
}

void VariableModelData::ClearChildren()
{
    for (auto *c : child_map_)
    {
        if (c)
            c->parent_ = nullptr;
    }
    child_map_.clear();
    child_list_.clear();
}

int VariableModelData::IndexOfChild(VariableModelData *child) const
{
    if (!child)
        return -1;
    int i = 0;
    for (auto it = child_map_.cbegin(); it != child_map_.cend(); ++it, ++i)
    {
        if (it.value() == child)
            return i;
    }
    return -1;
}

VariableModelDataManager::VariableModelDataManager(QObject *parent) : QObject(parent) {}

VariableModelDataManager::~VariableModelDataManager()
{
    Clear();
}

VariableModelData *VariableModelDataManager::CreateData(const QString &name, const QString &value, const QString &type)
{
    if (data_map_.contains(name))
        return nullptr;

    VariableModelData *raw = new VariableModelData(name, value, type);
    raw->manager_ = this;
    data_map_.insert(name, raw);
    return raw;
}

VariableModelData *VariableModelDataManager::GetData(const QString &name) const
{
    auto it = data_map_.find(name);
    return (it == data_map_.end()) ? nullptr : it.value();
}

bool VariableModelDataManager::RemoveData(VariableModelData *data)
{
    if (!data)
        return false;

    auto it = data_map_.find(data->name_);
    if (it == data_map_.end() || it.value() != data)
        return false;

    QString name = data->name_;
    data_map_.erase(it);
    return true;
}

bool VariableModelDataManager::RemoveData(const QString &name)
{
    auto it = data_map_.find(name);
    if (it == data_map_.end())
        return false;

    return RemoveData(it.value());
}

void VariableModelDataManager::Clear()
{
    // delete
    for (auto *data : data_map_)
    {
        delete data;
    }
    data_map_.clear();
}

} // namespace gui
} // namespace xequation