#pragma once

#include "core/equation.h"
#include "core/equation_common.h"
#include "core/equation_signals_manager.h"
#include "value_model_view/value_item.h"
#include "value_model_view/value_tree_model.h"
#include "value_model_view/value_tree_view.h"

#include <functional>
#include <map>
#include <boost/bimap.hpp>

namespace xequation
{
namespace gui
{
class ExpressionWatchModel : public ValueTreeModel
{
    Q_OBJECT
public:
    ExpressionWatchModel(QObject* parent = nullptr);
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool IsPlaceHolderIndex(const QModelIndex& index) const;
    void AddWatchItem(ValueItem* item);
    void RemoveWatchItem(ValueItem* item);
    void ReplaceWatchItem(ValueItem* old_item, ValueItem* new_item);
signals:
    void RequestAddWatchItem(const QString& expression);
    void RequestRemoveWatchItem(ValueItem* item);
    void RequestReplaceWatchItem(ValueItem* old_item, const QString& new_expression);
private:
    void* placeholder_flag_ = nullptr;
};

class ExpressionWatchWidget : public QWidget
{
public:
    using EvalExprHandler = std::function<InterpretResult(const std::string&)>;
    using ParseExprHandler = std::function<ParseResult(const std::string&)>;
    ExpressionWatchWidget(EvalExprHandler eval_handler, ParseExprHandler parse_handler, QWidget* parent = nullptr);
    ~ExpressionWatchWidget() = default;
    void OnEquationAdded(const Equation* equation);
    void OnEquationRemoving(const Equation* equation);
    void OnEquationUpdated(const Equation* equation, bitmask::bitmask<EquationUpdateFlag> change_type);
protected:
    void SetupUI();
    void SetupConnections();
    ValueItem* CreateWatchItem(const QString& expression);
    void DeleteWatchItem(ValueItem* item);
    void OnRequestAddWatchItem(const QString& expression);
    void OnRequestRemoveWatchItem(ValueItem* item);
    void OnRequestReplaceWatchItem(ValueItem* old_item, const QString& new_expression);
private:
    typedef boost::bimaps::bimap<
        boost::bimaps::set_of<ValueItem*>,
        boost::bimaps::set_of<std::string>
    > ExpressionItemEquationNameBimap;
    ExpressionWatchModel* model_;
    ValueTreeView* view_;
    EvalExprHandler eval_handler_ = nullptr;
    ParseExprHandler parse_handler_ = nullptr;
    ExpressionItemEquationNameBimap expression_item_equation_name_bimap_;
    std::multimap<std::string, ValueItem::UniquePtr> expression_item_map_;
};
} // namespace gui
} // namespace xequation