#include "equation_editor.h"
#include "core/equation.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
bool ContextFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    auto *src = sourceModel();
    QModelIndex idx = src->index(source_row, 0, source_parent);
    QString word = src->data(idx, LanguageModel::kWordRole).toString();
    if (word.isEmpty())
    {
        return true;
    }

    if (!category_.isEmpty())
    {
        QString category = src->data(idx, LanguageModel::kCategoryRole).toString();
        if (category != category_)
        {
            return false;
        }
    }

    if (!filter_text_.isEmpty())
    {
        if (!word.contains(filter_text_, Qt::CaseInsensitive))
        {
            return false;
        }
    }

    return EquationGroupFilterModel::filterAcceptsRow(source_row, source_parent);
}
    
ContextSelectionWidget::ContextSelectionWidget(EquationLanguageModel* language_model, QWidget *parent)
    : QWidget(parent), language_model_(language_model)
{
    SetupUI();
    SetupConnections();
}

void ContextSelectionWidget::SetupUI()
{
    QVBoxLayout *main_layout = new QVBoxLayout(this);

    context_combo_box_ = new QComboBox(this);
    auto keys = data_map_.keys();
    for (const auto &key : keys)
    {
        context_combo_box_->addItem(key);
    }

    context_filter_edit_ = new QLineEdit(this);
    context_filter_edit_->setPlaceholderText("Filter variables...");
    context_list_widget_ = new QListWidget(this);

    main_layout->addWidget(context_combo_box_);
    main_layout->addWidget(context_filter_edit_);
    main_layout->addWidget(context_list_widget_);

    context_combo_box_->setCurrentText(data_map_.firstKey());
    UpdateListWidget(data_map_.first());
    adjustSize();
    main_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

void ContextSelectionWidget::SetupConnections()
{
    connect(context_combo_box_, &QComboBox::currentTextChanged, this, &ContextSelectionWidget::OnComboBoxChanged);
    connect(context_filter_edit_, &QLineEdit::textChanged, this, &ContextSelectionWidget::OnFilterTextChanged);
}

QString ContextSelectionWidget::GetSelectedVariable() const
{
    QListWidgetItem *current_item = context_list_widget_->currentItem();
    if (current_item)
    {
        return current_item->text();
    }
    return QString();
}

void ContextSelectionWidget::UpdateListWidget(const QList<QString> &data_list)
{
    context_list_widget_->clear();
    for (const auto &item : data_list)
    {
        context_list_widget_->addItem(item);
    }
}

void ContextSelectionWidget::OnComboBoxChanged(const QString &text)
{
    if (data_map_.contains(text))
    {
        UpdateListWidget(data_map_.value(text));
    }
}

void ContextSelectionWidget::OnFilterTextChanged(const QString &text)
{
    QString current_key = context_combo_box_->currentText();
    if (data_map_.contains(current_key))
    {
        QList<QString> filtered_list;
        for (const auto &item : data_map_.value(current_key))
        {
            if (item.contains(text, Qt::CaseInsensitive))
            {
                filtered_list.append(item);
            }
        }
        UpdateListWidget(filtered_list);
    }
}

EquationEditor::EquationEditor(EquationLanguageModel* language_model, QWidget *parent)
    : QDialog(parent), group_(nullptr), language_model_(language_model)
{
    SetupUI();
    SetupConnections();
}

void EquationEditor::SetEquationGroup(const EquationGroup* group)
{
    group_ = group;
    if (group_ != nullptr)
    {
        const auto &equation_names = group_->GetEquationNames();
        if (equation_names.size() == 1)
        {
            const auto &equation = group_->GetEquation(equation_names[0]);
            equation_name_edit_->setText(QString::fromStdString(equation->name()));
            expression_edit_->setText(QString::fromStdString(equation->content()));
        }
        else 
        {
            equation_name_edit_->clear();
            expression_edit_->clear();
        }
    }
}

void EquationEditor::SetupUI()
{
    equation_name_label_ = new QLabel("Name:", this);
    equation_name_edit_ = new QLineEdit(this);
    expression_label_ = new QLabel("Expression:", this);
    expression_edit_ = new QLineEdit(this);
    insert_button_ = new QPushButton("Insert", this);
    ok_button_ = new QPushButton("OK", this);
    cancel_button_ = new QPushButton("Cancel", this);

    context_button_ = new QPushButton("Context>>", this);
    QMap<QString, QList<QString>> data_map;
    if (manager_)
    {
        context_button_->setVisible(true);
        auto equation_names = manager_->GetEquationNames();
        QList<QString> equation_list;
        for (const auto &name : equation_names)
        {
            equation_list.append(QString::fromStdString(name));
        }
        data_map.insert("Equation", equation_list);

        auto variable_names = manager_->GetExternalVariableNames();
        QList<QString> variable_list;
        for (const auto &name : variable_names)
        {
            variable_list.append(QString::fromStdString(name));
        }
        data_map.insert("Variable", variable_list);
    }
    else
    {
        context_button_->setVisible(false);
    }
    context_selection_widget_ = new ContextSelectionWidget(data_map, this);
    context_selection_widget_->setVisible(false);
    insert_button_->setVisible(false);

    QVBoxLayout *main_layout = new QVBoxLayout();
    QGridLayout *equation_layout = new QGridLayout();
    QVBoxLayout *context_layout = new QVBoxLayout();
    QHBoxLayout *context_button_layout = new QHBoxLayout();
    QHBoxLayout *button_layout = new QHBoxLayout();

    equation_layout->addWidget(equation_name_label_, 0, 0);
    equation_layout->addWidget(equation_name_edit_, 0, 1);
    equation_layout->addWidget(expression_label_, 1, 0);
    equation_layout->addWidget(expression_edit_, 1, 1);

    context_button_layout->addStretch();
    context_button_layout->addWidget(context_button_);
    context_layout->addLayout(context_button_layout);
    context_layout->addWidget(context_selection_widget_);

    button_layout->addStretch();
    button_layout->addWidget(insert_button_);
    button_layout->addWidget(ok_button_);
    button_layout->addWidget(cancel_button_);

    main_layout->addLayout(equation_layout);
    main_layout->addLayout(context_layout);
    main_layout->addLayout(button_layout);
    setLayout(main_layout);

    setWindowTitle("Insert Equation");
    adjustSize();
    setMinimumWidth(500);
    main_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

void EquationEditor::SetupConnections()
{
    connect(context_button_, &QPushButton::clicked, this, &EquationEditor::OnContextButtonClicked);
    connect(insert_button_, &QPushButton::clicked, this, &EquationEditor::OnInsertButtonClicked);
    connect(ok_button_, &QPushButton::clicked, this, &EquationEditor::OnOkButtonClicked);
    connect(cancel_button_, &QPushButton::clicked, this, &EquationEditor::OnCancelButtonClicked);
}

void EquationEditor::OnContextButtonClicked()
{
    if (context_button_->text().contains(">>"))
    {
        context_selection_widget_->setVisible(true);
        insert_button_->setVisible(true);
        context_button_->setText("Context<<");
    }
    else
    {
        context_selection_widget_->setVisible(false);
        insert_button_->setVisible(false);
        context_button_->setText("Context>>");
    }
}

void EquationEditor::OnInsertButtonClicked()
{
    QString select_variable = context_selection_widget_->GetSelectedVariable();
    if (select_variable.isEmpty() == false)
    {
        expression_edit_->insert(select_variable);
    }
}

void EquationEditor::OnOkButtonClicked()
{
    QString name = equation_name_edit_->text();
    QString expression = expression_edit_->text();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "equation name is empty!", QMessageBox::Ok);
        return;
    }

    if (expression.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "equation expression is empty", QMessageBox::Ok);
        return;
    }

    QString statement = name + " = " + expression;
    if (manager_->IsStatementSingleEquation(statement.toStdString()) == false)
    {
        QMessageBox::warning(
            this, "Warning", "equation insert editor only support insert single equation!", QMessageBox::Ok
        );
        return;
    }

    if (mode_ == Mode::kInsert)
    {
        emit AddEquationRequest(statement);
    }
    else
    {
        emit EditEquationRequest(group_->id(), statement);
    }
}

void EquationEditor::OnCancelButtonClicked()
{
    reject();
}

void EquationEditor::OnSuccess()
{
    accept();
}

} // namespace gui
} // namespace xequation