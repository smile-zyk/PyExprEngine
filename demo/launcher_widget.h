#pragma once

#include <QJsonObject>
#include <QWidget>
#include <QUuid>

#include "host/postprocess_manager.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace xresults
{
namespace gui
{

// =========================================================================
// LauncherWidget -- a demo host application.
//
// It does no post-processing itself: it uses PostprocessManager to launch
// several `postprocess` child processes and to drive each of them
// independently over JSON-RPC 2.0.
//
// Every command goes to the instance selected in the left list, so the
// "different commands to different processes" flow is: select #1 -> add
// equation -> select #2 -> raise -> ...
// =========================================================================

class LauncherWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit LauncherWidget(QWidget *parent = nullptr);
    ~LauncherWidget() override;

  private:
    void SetupUI();
    void SetupConnections();

    void OnLaunch();
    void OnStopSelected();
    void StopAll();

    /// Id of the selected instance, or a null QUuid when nothing is selected.
    QUuid CurrentInstanceId() const;

    void RefreshInstanceList();
    void AppendLog(const QString &text);
    host::LaunchConfig BuildLaunchConfig() const;

    // ---- per-instance commands ----------------------------------------
    void OnRaise();
    void OnAddEquation();
    void OnAddExpression();
    void OnAddDataset();
    void OnSetDefaultDataset();
    void OnQueryState();

    void ReportResult(const QString &what, bool ok, const QString &error);

  private:
    QLineEdit *title_edit_ = nullptr;
    QPushButton *launch_button_ = nullptr;
    QPushButton *stop_button_ = nullptr;
    QPushButton *stop_all_button_ = nullptr;

    QListWidget *instance_list_ = nullptr;

    QLineEdit *equation_name_edit_ = nullptr;
    QLineEdit *equation_content_edit_ = nullptr;
    QLineEdit *expression_edit_ = nullptr;
    QLineEdit *dataset_name_edit_ = nullptr;
    QLineEdit *dataset_path_edit_ = nullptr;
    QComboBox *dataset_format_combo_ = nullptr;
    QComboBox *default_dataset_combo_ = nullptr;
    QPushButton *raise_button_ = nullptr;
    QPushButton *add_equation_button_ = nullptr;
    QPushButton *add_expression_button_ = nullptr;
    QPushButton *add_dataset_button_ = nullptr;
    QPushButton *browse_dataset_button_ = nullptr;
    QPushButton *set_default_button_ = nullptr;
    QPushButton *query_state_button_ = nullptr;

    QPlainTextEdit *log_view_ = nullptr;
    QLabel *status_label_ = nullptr;

    host::PostprocessManager *manager_ = nullptr;
};

} // namespace gui
} // namespace xresults
