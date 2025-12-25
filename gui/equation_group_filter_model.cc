#include "equation_group_filter_model.h"
#include "equation_language_model.h"

namespace xequation {
namespace gui {

bool EquationGroupFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	auto *src = sourceModel();
	// Only apply filtering when the source model is LanguageModel
	auto *langModel = qobject_cast<const EquationLanguageModel *>(src);
	if (!langModel)
	{
		return true;
	}

	if (!group_)
	{
		return true;
	}

	QModelIndex idx = src->index(source_row, 0, source_parent);
	QString word = src->data(idx, LanguageModel::kWordRole).toString();
	if (word.isEmpty())
	{
		return true;
	}

	// Filter out items whose word exists as an equation name in the group
	if (group_->IsEquationExist(word.toStdString()))
	{
		return false;
	}

	return true;
}

} // namespace gui
} // namespace xequation
