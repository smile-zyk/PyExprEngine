#pragma once

#include <QtVariantProperty>

#include "core/equation.h"
#include "qtvariantproperty.h"

namespace xequation
{
namespace gui
{
    class EquationPropertyItem : public xequation::EquationObserver
    {
    public:
        EquationPropertyItem(const Equation* equation, QtVariantPropertyManager* manager);
        ~EquationPropertyItem() override;

        void SetupProperties();

        void SetEquation(const Equation* equation);

        void ClearSubProperties(QtProperty* property);

        QtVariantProperty* main_property() const { return main_property_; }
        QtVariantProperty* content_property() const { return content_property_; }
        QtVariantProperty* type_property() const { return type_property_; }
        QtVariantProperty* status_property() const { return status_property_; }
        QtVariantProperty* message_property() const { return message_property_; }
        QtVariantProperty* dependencies_property() const { return dependencies_property_; }

        void OnEquationFieldChanged(const Equation* equation, const std::string& field_name) override;
    private:
        QtVariantPropertyManager* attribute_property_manager_;
        QtVariantProperty* main_property_ = nullptr;
        QtVariantProperty* content_property_ = nullptr;
        QtVariantProperty* type_property_ = nullptr;
        QtVariantProperty* status_property_ = nullptr;
        QtVariantProperty* message_property_ = nullptr;
        QtVariantProperty* dependencies_property_ = nullptr;
    };
}
}