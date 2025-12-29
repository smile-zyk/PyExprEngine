#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

#include "core/equation_group.h"
#include "equation_language_model.h"
#include "equation_group_filter_model.h"

namespace xequation
{
namespace gui
{

class ContextFilterModel: public EquationGroupFilterModel
{
    Q_OBJECT
  public:
    explicit ContextFilterModel(const EquationGroup* group, QObject* parent = nullptr)
        : EquationGroupFilterModel(group, parent)
    {
    }

    void SetCategory(const QString &category)
    {
        category_ = category;
        invalidateFilter();
    }

    void SetFilterText(const QString &filter_text)
    {
        filter_text_ = filter_text;
        invalidateFilter();
    }

  protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

  private:
    QString category_;
    QString filter_text_;
};

class ContextSelectionWidget : public QWidget
{
    Q_OBJECT
  public:
    ContextSelectionWidget(EquationLanguageModel* language_model, QWidget *parent = nullptr);
    ~ContextSelectionWidget() {}
    QString GetSelectedVariable() const;
    void OnComboBoxChanged(const QString &text);
    void OnFilterTextChanged(const QString &text);

  protected:
    void SetupUI();
    void SetupConnections();

  private:
    EquationLanguageModel* language_model_{};
    QComboBox *context_combo_box_;
    QLineEdit *context_filter_edit_;
    QListView *context_list_widget_;
};

class EquationEditor : public QDialog
{
    Q_OBJECT
  public:
    EquationEditor(EquationLanguageModel* language_model, QWidget *parent = nullptr);
    ~EquationEditor() {}
    void SetEquationGroup(const EquationGroup* group);

  signals:
    void AddEquationRequest(const QString& statement);
    void EditEquationRequest(const EquationGroupId& group_id, const QString& statement);

  private:
    void SetupUI();
    void SetupConnections();
    void OnContextButtonClicked();
    void OnInsertButtonClicked();
    void OnOkButtonClicked();
    void OnCancelButtonClicked();

  private:
    const EquationGroup* group_{};
    EquationLanguageModel* language_model_{};
    QLabel *equation_name_label_{};
    QLineEdit *equation_name_edit_{};
    QLabel *expression_label_{};
    QLineEdit *expression_edit_{};
    QPushButton *context_button_{};
    ContextSelectionWidget *context_selection_widget_{};
    QPushButton *insert_button_{};
    QPushButton *ok_button_{};
    QPushButton *cancel_button_{};
};
} // namespace gui
} // namespace xequation