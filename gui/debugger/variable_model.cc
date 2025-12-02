#include "variable_model.h"
#include "variable_manager.h"

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

void VariableModel::SetRootData(const QList<Variable *> &root_data)
{
    beginResetModel();
    root_data_ = root_data;
    endResetModel();
}

void VariableModel::AddRootData(Variable *data)
{
    if (!data) return;
    int row = root_data_.size();
    beginInsertRows(QModelIndex(), row, row);
    root_data_.append(data);
    endInsertRows();
}

void VariableModel::RemoveRootData(Variable *data)
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

Variable *VariableModel::GetRootData() const
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

    auto *parent_data = GetVariableFromIndex(parent);
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

    auto *parent_data = GetVariableFromIndex(parent);
    if (!parent_data) return QModelIndex();

    auto *child = parent_data->GetChildAt(row);
    if (!child) return QModelIndex();

    return createIndex(row, column, child);
}

QModelIndex VariableModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();

    auto *child_data = GetVariableFromIndex(child);
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

    auto *d = GetVariableFromIndex(index);
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

Variable *VariableModel::GetVariableFromIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<Variable *>(index.internalPointer()) : nullptr;
}

QModelIndex VariableModel::GetIndexFromVariable(Variable *data) const
{
    if (!data) return QModelIndex();

    int idx = root_data_.indexOf(data);
    if (idx >= 0)
        return createIndex(idx, 0, data);

    auto *parent_data = data->parent();
    if (!parent_data) return QModelIndex();

    int row = RowOfChildInParent(parent_data, data);
    if (row < 0) return QModelIndex();

    QModelIndex parent_index = GetIndexFromVariable(parent_data);
    if (!parent_index.isValid()) return QModelIndex();

    return createIndex(row, 0, data);
}

int VariableModel::RowOfChildInParent(Variable *parent, Variable *child) const
{
    if (!parent || !child) return -1;
    const auto children = parent->children();
    return children.indexOf(child);
}

} // namespace gui
} // namespace xequation