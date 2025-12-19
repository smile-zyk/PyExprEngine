#include "expression_watch_widget.h"

#include "core/equation_common.h"
#include "value_model_view/value_item.h"
#include "value_model_view/value_item_builder.h"
#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
ExpressionWatchModel::ExpressionWatchModel(QObject *parent) : ValueTreeModel(parent)
{
    placeholder_flag_ = reinterpret_cast<void *>(0x1);
}

QModelIndex ExpressionWatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row == root_items_.size())
    {
        return createIndex(row, column, placeholder_flag_);
    }
    return ValueTreeModel::index(row, column, parent);
}

QModelIndex ExpressionWatchModel::parent(const QModelIndex &child) const
{
    if (IsPlaceHolderIndex(child))
    {
        return QModelIndex();
    }
    return ValueTreeModel::parent(child);
}

int ExpressionWatchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return root_items_.size() + 1;
    }
    return ValueTreeModel::rowCount(parent);
}

QVariant ExpressionWatchModel::data(const QModelIndex &index, int role) const
{
    if (IsPlaceHolderIndex(index))
    {
        if (index.column() == 0)
        {
            switch (role)
            {
            case Qt::DisplayRole:
                return "Add item to watch...";
            case Qt::EditRole:
                return "";
            case Qt::ForegroundRole:
                return QColor(Qt::gray);
            case Qt::FontRole: {
                QFont font;
                font.setItalic(true);
                return font;
            }
            default:
                return QVariant();
            }
        }
        return QVariant();
    }
    return ValueTreeModel::data(index, role);
}

bool ExpressionWatchModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // The root has an extra placeholder child
        return true;
    }
    if (IsPlaceHolderIndex(parent))
    {
        return false;
    }
    return ValueTreeModel::hasChildren(parent);
}

bool ExpressionWatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;
    QString new_expression = value.toString().trimmed();

    if (new_expression.isEmpty())
    {
        return false;
    }

    if (IsPlaceHolderIndex(index))
    {
        emit RequestAddWatchItem(new_expression);
        return true;
    }

    int row = index.row();
    if (row < 0 || row >= root_items_.size())
    {
        return false;
    }

    QString current_expression = root_items_[row]->name();
    if (current_expression == new_expression)
    {
        return false;
    }
    emit RequestReplaceWatchItem(root_items_[row], new_expression);
    return true;
}

Qt::ItemFlags ExpressionWatchModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0 && !index.parent().isValid())
    {
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ExpressionWatchModel::IsPlaceHolderIndex(const QModelIndex &index) const
{
    return index.row() == root_items_.size() && index.isValid() && index.internalPointer() == placeholder_flag_;
}

void ExpressionWatchModel::AddWatchItem(ValueItem *item)
{
    if (!item)
        return;

    int row = root_items_.size();
    beginInsertRows(QModelIndex(), row, row);
    root_items_.push_back(item);
    endInsertRows();
}

void ExpressionWatchModel::RemoveWatchItem(ValueItem *item)
{
    if (!item)
        return;

    int index = root_items_.indexOf(item);
    if (index < 0)
        return;

    beginRemoveRows(QModelIndex(), index, index);
    root_items_.removeAt(index);
    endRemoveRows();
}

void ExpressionWatchModel::ReplaceWatchItem(ValueItem *old_item, ValueItem *new_item)
{
    if (!old_item || !new_item)
        return;

    int index = root_items_.indexOf(old_item);
    if (index < 0)
        return;

    beginRemoveRows(QModelIndex(), index, index);
    root_items_.removeAt(index);
    endRemoveRows();

    beginInsertRows(QModelIndex(), index, index);
    root_items_.insert(index, new_item);
    endInsertRows();
}

ExpressionWatchWidget::ExpressionWatchWidget(
    EvalExprHandler eval_handler, ParseExprHandler parse_handler, QWidget *parent
)
    : eval_handler_(eval_handler), parse_handler_(parse_handler), QWidget(parent)
{
    SetupUI();
    SetupConnections();
}

void ExpressionWatchWidget::OnEquationAdded(const Equation *equation)
{
    // find all watch items depending on this equation
    auto range = expression_item_equation_name_bimap_.right.equal_range(equation->name());
    for (auto it = range.first; it != range.second; ++it)
    {
        ValueItem *item = it->get_left();
        auto expression = item->name();
        ;
        // recreate the watch item
        auto new_item = CreateWatchItem(expression);
        if (new_item)
        {
            model_->ReplaceWatchItem(item, new_item);
            DeleteWatchItem(item);
        }
    }
}

void ExpressionWatchWidget::OnEquationRemoving(const Equation *equation) {}

void ExpressionWatchWidget::OnEquationUpdated(
    const Equation *equation, bitmask::bitmask<EquationUpdateFlag> change_type
)
{
    // find all watch items depending on this equation
    auto range = expression_item_equation_name_bimap_.right.equal_range(equation->name());
    for (auto it = range.first; it != range.second; ++it)
    {
        ValueItem *item = it->get_left();
        auto expression = item->name();
        ;
        // recreate the watch item
        auto new_item = CreateWatchItem(expression);
        if (new_item)
        {
            model_->ReplaceWatchItem(item, new_item);
            DeleteWatchItem(item);
        }
    }
}

void ExpressionWatchWidget::SetupUI()
{
    setWindowTitle("Expression Watch");
    setWindowFlags(
        Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint
    );

    view_ = new ValueTreeView(this);
    model_ = new ExpressionWatchModel(view_);

    view_->SetValueModel(model_);
    view_->SetHeaderSectionResizeRatio(0, 1);
    view_->SetHeaderSectionResizeRatio(1, 3);
    view_->SetHeaderSectionResizeRatio(2, 1);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(view_);
    setLayout(main_layout);

    setMinimumSize(800, 600);
}

void ExpressionWatchWidget::SetupConnections()
{
    connect(model_, &ExpressionWatchModel::RequestAddWatchItem, this, &ExpressionWatchWidget::OnRequestAddWatchItem);

    connect(
        model_, &ExpressionWatchModel::RequestRemoveWatchItem, this, &ExpressionWatchWidget::OnRequestRemoveWatchItem
    );

    connect(
        model_, &ExpressionWatchModel::RequestReplaceWatchItem, this, &ExpressionWatchWidget::OnRequestReplaceWatchItem
    );
}

ValueItem *ExpressionWatchWidget::CreateWatchItem(const QString &expression)
{
    if (eval_handler_ == nullptr || parse_handler_ == nullptr)
    {
        return nullptr;
    }

    ParseResult parse_result = parse_handler_(expression.toStdString());
    if (parse_result.items.size() != 1)
    {
        return nullptr;
    }

    ValueItem::UniquePtr item;
    const ParseResultItem &parse_item = parse_result.items[0];
    if (parse_item.status != ResultStatus::kSuccess)
    {
        item = ValueItem::Create(
            expression, QString::fromStdString(parse_item.message),
            QString::fromStdString(ResultStatusConverter::ToString(parse_item.status))
        );
    }
    else
    {
        InterpretResult interpret_result = eval_handler_(expression.toStdString());
        if (interpret_result.status != ResultStatus::kSuccess)
        {
            item = ValueItem::Create(
                expression, QString::fromStdString(interpret_result.message),
                QString::fromStdString(ResultStatusConverter::ToString(interpret_result.status))
            );
        }
        else
        {
            Value value = interpret_result.value;
            item = BuilderUtils::CreateValueItem(expression, value);
        }
        const auto &dependencies = parse_item.dependencies;
        for (const auto &dependency : dependencies)
        {
            expression_item_equation_name_bimap_.insert({item.get(), dependency});
        }
    }
    auto item_ptr = item.get();
    expression_item_map_.insert({expression.toStdString(), std::move(item)});
    return item_ptr;
}

void ExpressionWatchWidget::DeleteWatchItem(ValueItem *item)
{
    if (!item)
        return;

    auto it = expression_item_equation_name_bimap_.left.find(item);
    if (it != expression_item_equation_name_bimap_.left.end())
    {
        expression_item_equation_name_bimap_.left.erase(it);
    }

    auto expression = item->name().toStdString();
    auto range = expression_item_map_.equal_range(expression);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second.get() == item)
        {
            expression_item_map_.erase(it);
            break;
        }
    }
}

void ExpressionWatchWidget::OnRequestAddWatchItem(const QString &expression)
{
    auto item = CreateWatchItem(expression);
    if (!item)
        return;
    model_->AddWatchItem(item);
}

void ExpressionWatchWidget::OnRequestRemoveWatchItem(ValueItem *item)
{
    model_->RemoveWatchItem(item);
    DeleteWatchItem(item);
}

void ExpressionWatchWidget::OnRequestReplaceWatchItem(ValueItem *old_item, const QString &new_expression)
{
    auto new_item = CreateWatchItem(new_expression);
    if (!new_item)
        return;
    model_->ReplaceWatchItem(old_item, new_item);
    DeleteWatchItem(old_item);
}

} // namespace gui
} // namespace xequation