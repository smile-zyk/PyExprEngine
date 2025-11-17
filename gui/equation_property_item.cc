#include "equation_property_item.h"
#include "core/equation_manager.h"

namespace xequation
{
namespace gui
{
EquationPropertyItem::EquationPropertyItem(const Equation *equation, QtVariantPropertyManager *manager)
{
    attribute_property_manager_ = manager;
    SetupProperties();
    SetEquation(equation);
}

EquationPropertyItem::~EquationPropertyItem() {}

void EquationPropertyItem::SetupProperties()
{
    main_property_ = static_cast<QtVariantProperty *>(
        attribute_property_manager_->addProperty(QtVariantPropertyManager::groupTypeId(), "Equation")
    );

    content_property_ =
        static_cast<QtVariantProperty *>(attribute_property_manager_->addProperty(QVariant::String, "Content"));
    type_property_ =
        static_cast<QtVariantProperty *>(attribute_property_manager_->addProperty(QVariant::String, "Type"));
    status_property_ =
        static_cast<QtVariantProperty *>(attribute_property_manager_->addProperty(QVariant::String, "Status"));
    message_property_ =
        static_cast<QtVariantProperty *>(attribute_property_manager_->addProperty(QVariant::String, "Message"));
    dependencies_property_ = static_cast<QtVariantProperty *>(
        attribute_property_manager_->addProperty(QtVariantPropertyManager::groupTypeId(), "Dependencies")
    );

    main_property_->setVisible(true);
    content_property_->setVisible(true);
    type_property_->setVisible(true);
    status_property_->setVisible(true);
    message_property_->setVisible(true);
    dependencies_property_->setVisible(true);

    main_property_->addSubProperty(content_property_);
    main_property_->addSubProperty(type_property_);
    main_property_->addSubProperty(status_property_);
    main_property_->addSubProperty(message_property_);
    main_property_->addSubProperty(dependencies_property_);
}

void EquationPropertyItem::SetEquation(const Equation *equation)
{
    if (equation)
    {
        main_property_->setPropertyName(QString::fromStdString(equation->name()));
        content_property_->setValue(QString::fromStdString(equation->content()));
        type_property_->setValue(QString::fromStdString(Equation::TypeToString(equation->type())));
        status_property_->setValue(QString::fromStdString(Equation::StatusToString(equation->status())));
        message_property_->setValue(QString::fromStdString(equation->message()));
        ClearSubProperties(dependencies_property_);
        for(const std::string& dep : equation->dependencies())
        {
            QtVariantProperty* dep_property = static_cast<QtVariantProperty *>(
                attribute_property_manager_->addProperty(QVariant::String, QString::fromStdString(dep))
            );
            dep_property->setVisible(true);
            QString status = equation->manager()->IsEquationExist(dep) ? "Exist" : "Not Exist";
            dep_property->setValue(status);
            dependencies_property_->addSubProperty(dep_property);
        }
    }
}

void EquationPropertyItem::OnEquationFieldChanged(const Equation *equation, const std::string &field_name)
{
    if (field_name == "content" && content_property_)
    {
        content_property_->setValue(QString::fromStdString(equation->content()));
    }
    else if (field_name == "type" && type_property_)
    {
        type_property_->setValue(QString::fromStdString(Equation::TypeToString(equation->type())));
    }
    else if (field_name == "status" && status_property_)
    {
        status_property_->setValue(QString::fromStdString(Equation::StatusToString(equation->status())));
    }
    else if (field_name == "message" && message_property_)
    {
        message_property_->setValue(QString::fromStdString(equation->message()));
    }
    else if (field_name == "dependencies" && dependencies_property_) 
    {
        ClearSubProperties(dependencies_property_);
        for(const std::string& dep : equation->dependencies())
        {
            QtVariantProperty* dep_property = static_cast<QtVariantProperty *>(
                attribute_property_manager_->addProperty(QVariant::String, QString::fromStdString(dep))
            );
            QString status = equation->manager()->IsEquationExist(dep) ? "Exist" : "Not Exist";
            dep_property->setValue(status);
            dependencies_property_->addSubProperty(dep_property);
        }
    }
}

void EquationPropertyItem::ClearSubProperties(QtProperty* property)
{
    QList<QtProperty *> sub_properties = property->subProperties();

    for (QtProperty* sub_property : sub_properties)
    {
        property->removeSubProperty(sub_property);
    }
}

} // namespace gui
} // namespace xequation