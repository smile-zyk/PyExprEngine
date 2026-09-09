#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <string>
#include <vector>

#include "core/equation_common.h"
#include "core/equation_manager.h"
#include "core/equation_signals_manager.h"

class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QLabel;

namespace xresults
{
namespace gui
{

// =========================================================================
// PostprocessWidget -- a REL-engine demo built on EquationManager::GetInstance().
//
// Features:
//   + Input: name + expression (e.g. `y = [1, 2, 3]`); "Insert" inserts a
//     single Equation into the engine's EquationManager.
//   + "Redefine" / "Rename" buttons act on the selected equation.
//   - Middle-left: the engine's Equation list (QListWidget).
//   - Middle-right: DataFrameView, showing the selected Equation's
//     DataFrame table (lazy loading, fetchMore).
// =========================================================================

class PostprocessWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit PostprocessWidget(QWidget *parent = nullptr);
    ~PostprocessWidget() override;

    // =====================================================================
    // Programmatic API (shared by the GUI buttons and the RPC service)
    //
    // Every method reports failures by throwing std::runtime_error (or an
    // xequation::EquationException) instead of popping up a dialog, so the
    // RPC layer can turn them into JSON-RPC error objects.  The interactive
    // On*() slots below wrap them and show a QMessageBox instead.
    // =====================================================================

    /// Load a project file (datasets + python plugins + equations +
    /// expressions), replacing the currently loaded one.  Throws on failure.
    void LoadProjectOrThrow(const QString &path);

    /// Apply the inline parts of a startup config (the same schema as
    /// demo_project.json): datasets / default_dataset / equations /
    /// expressions.  Failures are collected into @p errors instead of thrown.
    void ApplyStartupConfig(const QJsonObject &config, QStringList *errors = nullptr);

    /// Add (or redefine, when @p redefine is true and the name exists) an
    /// equation.  Returns its ObjectId.
    xequation::ObjectId AddEquation(const QString &name, const QString &content,
                                    const QString &tag, bool redefine);

    /// Register a watch expression and open a tab for it.  Returns its id.
    xequation::ObjectId AddExpression(const QString &content, const QString &tag);

    /// Load one dataset file into the REL environment (format: "hdf5" /
    /// "touchstone") under @p name.  Throws on failure.
    void AddDataset(const QString &name, const QString &format, const QString &path,
                    bool make_default);

    /// Remove a dataset from the REL environment.
    void RemoveDataset(const QString &name);

    /// Make @p name the REL default dataset and recompute.
    void SetDefaultDataset(const QString &name);

    /// Names of all registered datasets (sorted), and the default one.
    std::vector<std::string> DatasetNames() const;
    QString DefaultDatasetName() const;

    /// Bring this window to the front (de-iconify + raise + activate).
    void RaiseWindow();

    /// Set the status-bar text.
    void SetStatusText(const QString &text);

  private:
    void SetupUI();
    void SetupConnections();

    void OnInsertEquation();
    void OnRedefineEquation();
    void OnRenameEquation();
    void OnDeleteEquation();
    void OnAddWatchExpression();
    void OnEquationListSelectionChanged();
    void RefreshEquationList();

    /// Enable / disable the Redefine / Rename / Delete buttons (both the
    /// equation-list and manager-tree selection handlers drive these).
    void UpdateEquationButtons(bool enabled);

    // ---- equation-manager tree panel ----------------------------------

    /// User selected a node in the manager tree (dataset / block / data array
    /// / equation / expression).  Routes the payload to the property widget
    /// and, for data arrays / equations / expressions, to the DataFrame tab.
    void OnManagerTreeSelectionChanged();
    void OnManagerTreeClicked();

    // ---- project file support ----------------------------------------

    /// Open a file dialog for a project file and load it (datasets +
    /// equations + expressions).
    void OnOpenProject();

    /// Save the current state (dataset refs + equations + expressions) to a
    /// project file via EquationManager::SaveToFile.
    void OnSaveProject();

    /// User picked another dataset in the combo -> make it the REL default
    /// dataset and recompute everything (bare DataArray names resolve against
    /// the default dataset).
    void OnDatasetSelectionChanged(int index);

    /// Rebuild the combo contents from rel::Environment::DatasetNames() and
    /// select the REL default dataset.
    void RefreshDatasetCombo();

    /// Loads a project file via EquationManager::LoadFromFile (referenced
    /// datasets + equations + expressions).  Datasets of a previously loaded
    /// project are dropped first so re-opening replaces the active set.  Also
    /// refreshes the combo / list / tree.  Shows a warning box on failure.
    void LoadProject(const QString &path);

    /// Name of the currently selected list item (item text is "name  [N row(s)]").
    QString CurrentSelectedEquationName() const;

    /// Select the list item by name (used to restore selection after edit/rename).
    void SelectEquationByName(const QString &name);

    /// Parse a "name = expr" input; returns name (empty = parse failed).
    static bool SplitStatement(const QString &statement, QString *name, QString *expr);

    /// Validate the name is a legal identifier (letters/digits/underscore,
    /// not starting with a digit).
    static bool IsValidIdentifier(const QString &name);

  private:
    QLineEdit *statement_edit_ = nullptr;
    QPushButton *insert_button_ = nullptr;
    QPushButton *redefine_button_ = nullptr;
    QPushButton *rename_button_ = nullptr;
    QPushButton *delete_button_ = nullptr;
    QPushButton *watch_button_ = nullptr;
    QPushButton *open_env_button_ = nullptr;
    QPushButton *save_project_button_ = nullptr;
    QComboBox *dataset_combo_ = nullptr;
    QLabel *status_label_ = nullptr;
    QListWidget *equation_list_ = nullptr;
    class ExplorerView *manager_tree_ = nullptr;
    class DataFrameTabWidget *data_frame_view_ = nullptr;
    class PropertyWidget *property_widget_ = nullptr;

    /// Absolute path of the last successfully loaded project file (empty =
    /// none).
    QString project_path_;

    /// Connection to the REL manager's kEquationRemoved signal: an equation
    /// left the manager (Delete button, or the manager-tree context menu
    /// which calls RemoveEquation directly) -- refresh the equation list so
    /// the middle-left panel stays in sync.
    xequation::ScopedConnection equation_removed_rel_connection_;
};

} // namespace gui
} // namespace xresults
