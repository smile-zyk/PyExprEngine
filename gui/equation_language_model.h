#pragma once

#include "code_editor/language_model.h"
#include "core/equation.h"

namespace xequation
{
namespace gui
{
class EquationLanguageModel : public LanguageModel
{
    Q_OBJECT
  public:
    explicit EquationLanguageModel(const QString &language_name, QObject *parent = nullptr)
        : LanguageModel(language_name, parent)
    {
    }
    ~EquationLanguageModel() override = default;

    void OnEquationAdded(const Equation *equation);
    void OnEquationRemoving(const Equation *equation);
};
} // namespace gui
} // namespace xequation