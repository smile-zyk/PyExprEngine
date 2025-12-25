#include "equation_language_model.h"
#include "core/equation_common.h"
#include <QString>

namespace xequation
{
namespace gui
{

void EquationLanguageModel::OnEquationAdded(const Equation *equation)
{
	if (!equation)
	{
		return;
	}

	QString word = QString::fromStdString(equation->name());
    QString type = QString::fromStdString(ItemTypeConverter::ToString(equation->type()));
	if(type == "Import" || type == "ImportFrom")
    {
        type = "Module";
    }
    AddWordItem(word, type, word);
}

void EquationLanguageModel::OnEquationRemoving(const Equation *equation)
{
	if (!equation)
	{
		return;
	}

	const QString word = QString::fromStdString(equation->name());
	RemoveWordItem(word);
}

} // namespace gui
} // namespace xequation
