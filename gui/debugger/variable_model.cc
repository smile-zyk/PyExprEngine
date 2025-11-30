#include "variable_model.h"
#include "variable_data_manager.h"

namespace xequation
{
namespace gui
{

VariableModel::VariableModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

VariableModel::~VariableModel()
{
}

void VariableModel::SetRootData(const QVector<VariableModelData *> &root_data)
{
    beginResetModel();
    root_data_ = root_data;
    endResetModel();
}

void VariableModel::AddRootData(VariableModelData *data)
{
    if (!data) return;
    int row = root_data_.size();
    beginInsertRows(QModelIndex(), row, row);
    root_data_.append(data);
    endInsertRows();
}

void VariableModel::RemoveRootData(VariableModelData *data)
{
    if (!data) return;
    int row = root_data_.indexOf(data);
    if (row < 0) return;
    
    beginRemoveRows(QModelIndex(), row, row);
    root_data_.removeAt(row);
    endRemoveRows();
}

void VariableModel::ClearRootData()
{
    if (root_data_.isEmpty()) return;
    
    beginResetModel();
    root_data_.clear();
    endResetModel();
}

VariableModelData *VariableModel::GetRootData() const
{
    return root_data_.isEmpty() ? nullptr : root_data_.first();
}

int VariableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3; // Name, Value, Type
}

int VariableModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return root_data_.size();

    auto *parent_data = GetDataFromIndex(parent);
    if (!parent_data) return 0;
    return parent_data->ChildCount();
}

QModelIndex VariableModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

    if (!parent.isValid())
    {
        if (row < 0 || row >= root_data_.size()) return QModelIndex();
        return createIndex(row, column, root_data_[row]);
    }

    auto *parent_data = GetDataFromIndex(parent);
    if (!parent_data) return QModelIndex();

    auto *child = parent_data->GetChildAt(row);
    if (!child) return QModelIndex();

    return createIndex(row, column, child);
}

QModelIndex VariableModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();

    auto *child_data = GetDataFromIndex(child);
    if (!child_data) return QModelIndex();

    auto *parent_data = child_data->parent();
    if (!parent_data) return QModelIndex();

    for (int i = 0; i < root_data_.size(); ++i)
    {
        if (root_data_[i] == parent_data)
            return createIndex(i, 0, parent_data);
    }

    auto *grand = parent_data->parent();
    if (!grand) return QModelIndex();

    int row = RowOfChildInParent(grand, parent_data);
    if (row < 0) return QModelIndex();

    return createIndex(row, 0, parent_data);
}

QVariant VariableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

    auto *d = GetDataFromIndex(index);
    if (!d) return QVariant();

    switch (index.column())
    {
    case 0: return d->name();
    case 1: return d->value();
    case 2: return d->type();
    default: return QVariant();
    }
}

QVariant VariableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0: return tr("Name");
        case 1: return tr("Value");
        case 2: return tr("Type");
        default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex VariableModel::CreateIndexFromData(VariableModelData *data, int row, int column) const
{
    if (!data) return QModelIndex();
    return createIndex(row, column, data);
}

VariableModelData *VariableModel::GetDataFromIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<VariableModelData *>(index.internalPointer()) : nullptr;
}

int VariableModel::RowOfChildInParent(VariableModelData *parent, VariableModelData *child) const
{
    if (!parent || !child) return -1;
    int i = 0;
    const auto children = parent->children();
    for (auto *c : children)
    {
        if (c == child) return i;
        ++i;
    }
    return -1;
}

} // namespace gui
} // namespace xequation