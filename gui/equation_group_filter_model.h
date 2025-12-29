#pragma once

#include <QSortFilterProxyModel>

#include "core/equation_group.h"

namespace xequation {
namespace gui
{
class EquationGroupFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit EquationGroupFilterModel(const EquationGroup* group, QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
        , group_(group)
    {
    }
    ~EquationGroupFilterModel() override = default;
    void SetEquationGroup(const EquationGroup* group)
    {
        group_ = group;
        invalidateFilter();
    }
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
private:
    const EquationGroup* group_;
};
}
}