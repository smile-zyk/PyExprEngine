#include "launcher_widget.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTime>
#include <QVBoxLayout>

#include "host/postprocess_instance.h"

namespace xresults
{
namespace gui
{
LauncherWidget::LauncherWidget(QWidget *parent) : QWidget(parent)
{
    manager_ = new host::PostprocessManager(this);
    SetupUI();
    SetupConnections();
    RefreshInstanceList();
}

LauncherWidget::~LauncherWidget()
{
    // Never leave orphan postprocess windows behind (the manager's destructor
    // also does this, but be explicit).
    StopAll();
}

void LauncherWidget::SetupUI()
{
    setWindowTitle("XEquation Launcher (host)");
    setMinimumSize(1000, 700);

    // ================= top: launch controls ============================
    QGroupBox *launch_box = new QGroupBox("Launch postprocess instance", this);
    title_edit_ = new QLineEdit(launch_box);
    title_edit_->setPlaceholderText("Window title (optional)");
    launch_button_ = new QPushButton("Launch", launch_box);
    stop_button_ = new QPushButton("Stop Selected", launch_box);
    stop_all_button_ = new QPushButton("Stop All", launch_box);
    stop_button_->setEnabled(false);

    QHBoxLayout *launch_layout = new QHBoxLayout(launch_box);
    launch_layout->addWidget(new QLabel("Title:", launch_box));
    launch_layout->addWidget(title_edit_, 1);
    launch_layout->addWidget(launch_button_);
    launch_layout->addWidget(stop_button_);
    launch_layout->addWidget(stop_all_button_);

    // ================= left: instance list =============================
    QGroupBox *instance_box = new QGroupBox("Instances", this);
    instance_list_ = new QListWidget(instance_box);
    instance_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *instance_layout = new QVBoxLayout(instance_box);
    instance_layout->addWidget(instance_list_);

    // ================= right: command panel ============================
    QGroupBox *command_box =
        new QGroupBox("Commands (sent to the selected instance)", this);

    raise_button_ = new QPushButton("Raise to Front", command_box);
    equation_name_edit_ = new QLineEdit(command_box);
    equation_name_edit_->setPlaceholderText("name (e.g. gain)");
    equation_content_edit_ = new QLineEdit(command_box);
    equation_content_edit_->setPlaceholderText(
        "content (e.g. LNA.amplifier.HB1.HB.Gain)");
    add_equation_button_ = new QPushButton("Add Equation", command_box);

    expression_edit_ = new QLineEdit(command_box);
    expression_edit_->setPlaceholderText(
        "expression (e.g. LNA.amplifier.HB1.HB.Pout)");
    add_expression_button_ = new QPushButton("Add Expression", command_box);

    dataset_name_edit_ = new QLineEdit(command_box);
    dataset_name_edit_->setPlaceholderText("dataset name (e.g. LQ)");
    dataset_path_edit_ = new QLineEdit(command_box);
    dataset_path_edit_->setPlaceholderText("dataset file path");
    browse_dataset_button_ = new QPushButton("Browse\u2026", command_box);
    dataset_format_combo_ = new QComboBox(command_box);
    dataset_format_combo_->addItem("hdf5");
    dataset_format_combo_->addItem("touchstone");
    add_dataset_button_ = new QPushButton("Add Dataset", command_box);

    default_dataset_combo_ = new QComboBox(command_box);
    default_dataset_combo_->setEnabled(false);
    set_default_button_ = new QPushButton("Set Default", command_box);
    query_state_button_ = new QPushButton("Query State", command_box);

    QFormLayout *form = new QFormLayout();
    form->addRow(raise_button_);
    form->addRow("Equation:", equation_name_edit_);
    form->addRow(equation_content_edit_, add_equation_button_);
    form->addRow("Expression:", expression_edit_);
    form->addRow(add_expression_button_);
    form->addRow("Dataset:", dataset_name_edit_);
    form->addRow(dataset_path_edit_, browse_dataset_button_);
    form->addRow("Format:", dataset_format_combo_);
    form->addRow(add_dataset_button_);
    form->addRow("Default:", default_dataset_combo_);
    form->addRow(set_default_button_, query_state_button_);

    QVBoxLayout *command_layout = new QVBoxLayout(command_box);
    command_layout->addLayout(form);
    command_layout->addStretch(1);

    // ================= bottom: log =====================================
    QGroupBox *log_box = new QGroupBox("JSON-RPC log", this);
    log_view_ = new QPlainTextEdit(log_box);
    log_view_->setReadOnly(true);
    log_view_->setMaximumBlockCount(2000);
    log_view_->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont log_font = log_view_->font();
    log_font.setFamily("Consolas");
    log_view_->setFont(log_font);
    QVBoxLayout *log_layout = new QVBoxLayout(log_box);
    log_layout->addWidget(log_view_);

    status_label_ = new QLabel("No instances. Press Launch to start one.", this);
    status_label_->setWordWrap(true);

    // ================= assemble ========================================
    QSplitter *top_splitter = new QSplitter(Qt::Horizontal, this);
    top_splitter->addWidget(instance_box);
    top_splitter->addWidget(command_box);
    top_splitter->setStretchFactor(0, 1);
    top_splitter->setStretchFactor(1, 2);
    top_splitter->setChildrenCollapsible(false);

    QSplitter *main_splitter = new QSplitter(Qt::Vertical, this);
    main_splitter->addWidget(top_splitter);
    main_splitter->addWidget(log_box);
    main_splitter->setStretchFactor(0, 2);
    main_splitter->setStretchFactor(1, 1);
    main_splitter->setChildrenCollapsible(false);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(launch_box);
    main_layout->addWidget(main_splitter, 1);
    main_layout->addWidget(status_label_);
    setLayout(main_layout);
}

void LauncherWidget::SetupConnections()
{
    connect(manager_, &host::PostprocessManager::LogMessage, this,
            &LauncherWidget::AppendLog);
    connect(manager_, &host::PostprocessManager::InstanceReady, this,
            [this](const QUuid &id) {
                RefreshInstanceList();
                status_label_->setText(
                    QString("Instance %1 is ready.").arg(id.toString().left(8)));
            });
    connect(manager_, &host::PostprocessManager::InstanceFinished, this,
            [this](const QUuid &id, int exit_code) {
                AppendLog(QString("instance %1 finished (exit %2)")
                              .arg(id.toString().left(8))
                              .arg(exit_code));
                RefreshInstanceList();
            });
    manager_->OnNotification(
        QStringLiteral("status_changed"),
        [this](const QUuid &id, const QJsonObject &params) {
            AppendLog(QString("[%1] status: %2")
                          .arg(id.toString().left(8))
                          .arg(params.value("text").toString()));
        });

    connect(launch_button_, &QPushButton::clicked, this, &LauncherWidget::OnLaunch);
    connect(stop_button_, &QPushButton::clicked, this, &LauncherWidget::OnStopSelected);
    connect(stop_all_button_, &QPushButton::clicked, this, &LauncherWidget::StopAll);
    connect(browse_dataset_button_, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, "Dataset File", QDir::currentPath(),
            "Datasets (*.h5 *.hdf5 *.s2p *.sNp);;All Files (*)");
        if (!path.isEmpty())
        {
            dataset_path_edit_->setText(path);
            if (dataset_name_edit_->text().trimmed().isEmpty())
            {
                dataset_name_edit_->setText(QFileInfo(path).completeBaseName());
            }
        }
    });

    connect(raise_button_, &QPushButton::clicked, this, &LauncherWidget::OnRaise);
    connect(add_equation_button_, &QPushButton::clicked, this,
            &LauncherWidget::OnAddEquation);
    connect(add_expression_button_, &QPushButton::clicked, this,
            &LauncherWidget::OnAddExpression);
    connect(add_dataset_button_, &QPushButton::clicked, this,
            &LauncherWidget::OnAddDataset);
    connect(set_default_button_, &QPushButton::clicked, this,
            &LauncherWidget::OnSetDefaultDataset);
    connect(query_state_button_, &QPushButton::clicked, this,
            &LauncherWidget::OnQueryState);

    connect(instance_list_, &QListWidget::itemSelectionChanged, this, [this]() {
        const bool has_selection = instance_list_->currentItem() != nullptr;
        stop_button_->setEnabled(has_selection);
        raise_button_->setEnabled(has_selection);
        add_equation_button_->setEnabled(has_selection);
        add_expression_button_->setEnabled(has_selection);
        add_dataset_button_->setEnabled(has_selection);
        set_default_button_->setEnabled(has_selection);
        query_state_button_->setEnabled(has_selection);
    });
}

// =========================================================================
// Instance management
// =========================================================================

host::LaunchConfig LauncherWidget::BuildLaunchConfig() const
{
    host::LaunchConfig config;
    config.title = title_edit_->text().trimmed();
    return config;
}

void LauncherWidget::OnLaunch()
{
    if (manager_->executable().isEmpty())
    {
        AppendLog("ERROR: postprocess executable not found next to this application.");
        status_label_->setText("postprocess executable not found.");
        return;
    }

    QString error;
    const QUuid id = manager_->Launch(BuildLaunchConfig(), &error);
    if (id.isNull())
    {
        AppendLog(QString("ERROR: %1").arg(error));
        status_label_->setText(error);
        return;
    }

    RefreshInstanceList();
    status_label_->setText(QString("Launched %1, waiting for ready\u2026")
                               .arg(id.toString().left(8)));
}

void LauncherWidget::OnStopSelected()
{
    const QUuid id = CurrentInstanceId();
    if (id.isNull())
    {
        return;
    }
    manager_->Stop(id);
    RefreshInstanceList();
}

void LauncherWidget::StopAll()
{
    manager_->StopAll();
    RefreshInstanceList();
}

QUuid LauncherWidget::CurrentInstanceId() const
{
    QListWidgetItem *item = instance_list_->currentItem();
    if (!item)
    {
        return QUuid();
    }
    return QUuid(item->data(Qt::UserRole).toString());
}

void LauncherWidget::RefreshInstanceList()
{
    const QString previous = CurrentInstanceId().toString();

    instance_list_->blockSignals(true);
    instance_list_->clear();

    for (const QUuid &id : manager_->ids())
    {
        host::PostprocessInstance *instance = manager_->instance(id);
        if (!instance)
        {
            continue;
        }
        const QString state = !instance->is_running()
                                  ? QStringLiteral("stopped")
                                  : (instance->is_ready() ? QStringLiteral("ready")
                                                          : QStringLiteral("starting"));
        QListWidgetItem *item = new QListWidgetItem(
            QString("%1  [%2, pid %3, %4]")
                .arg(instance->title())
                .arg(id.toString().left(8))
                .arg(instance->pid())
                .arg(state));
        item->setData(Qt::UserRole, id.toString());
        if (!instance->is_running())
        {
            item->setForeground(Qt::gray);
        }
        instance_list_->addItem(item);

        if (id.toString() == previous)
        {
            instance_list_->setCurrentItem(item);
        }
    }
    instance_list_->blockSignals(false);

    // Re-run the selection handler so the buttons follow the (possibly lost)
    // selection.
    Q_EMIT instance_list_->itemSelectionChanged();
}

void LauncherWidget::AppendLog(const QString &text)
{
    log_view_->appendPlainText(
        QStringLiteral("%1  %2")
            .arg(QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz")))
            .arg(text));
    log_view_->verticalScrollBar()->setValue(
        log_view_->verticalScrollBar()->maximum());
}

void LauncherWidget::ReportResult(const QString &what, bool ok, const QString &error)
{
    status_label_->setText(ok ? QString("%1 ok.").arg(what)
                              : QString("%1 failed: %2").arg(what, error));
}

// =========================================================================
// Per-instance commands
// =========================================================================

void LauncherWidget::OnRaise()
{
    const QUuid id = CurrentInstanceId();
    if (id.isNull())
    {
        return;
    }
    manager_->Call(id, QStringLiteral("window.raise"), {},
                   [this](bool ok, const QJsonValue &, const QString &error) {
                       ReportResult(QStringLiteral("Raise"), ok, error);
                   });
}

void LauncherWidget::OnAddEquation()
{
    const QUuid id = CurrentInstanceId();
    const QString name = equation_name_edit_->text().trimmed();
    const QString content = equation_content_edit_->text().trimmed();
    if (id.isNull())
    {
        return;
    }
    if (name.isEmpty() || content.isEmpty())
    {
        status_label_->setText("Equation needs both a name and content.");
        return;
    }

    QJsonObject params;
    params["name"] = name;
    params["content"] = content;
    params["redefine"] = true;

    manager_->Call(id, QStringLiteral("equation.add"), params,
                   [this, name](bool ok, const QJsonValue &, const QString &error) {
                       ReportResult(QString("Add equation %1").arg(name), ok, error);
                   });
}

void LauncherWidget::OnAddExpression()
{
    const QUuid id = CurrentInstanceId();
    const QString content = expression_edit_->text().trimmed();
    if (id.isNull())
    {
        return;
    }
    if (content.isEmpty())
    {
        status_label_->setText("Expression must not be empty.");
        return;
    }

    QJsonObject params;
    params["content"] = content;
    manager_->Call(id, QStringLiteral("expression.add"), params,
                   [this](bool ok, const QJsonValue &, const QString &error) {
                       ReportResult(QStringLiteral("Add expression"), ok, error);
                   });
}

void LauncherWidget::OnAddDataset()
{
    const QUuid id = CurrentInstanceId();
    const QString name = dataset_name_edit_->text().trimmed();
    const QString path = dataset_path_edit_->text().trimmed();
    if (id.isNull())
    {
        return;
    }
    if (name.isEmpty() || path.isEmpty())
    {
        status_label_->setText("Dataset needs both a name and a file path.");
        return;
    }

    QJsonObject params;
    params["name"] = name;
    params["path"] = path;
    params["format"] = dataset_format_combo_->currentText();
    params["make_default"] = true;

    manager_->Call(id, QStringLiteral("dataset.add"), params,
                   [this, name](bool ok, const QJsonValue &, const QString &error) {
                       ReportResult(QString("Add dataset %1").arg(name), ok, error);
                   });
}

void LauncherWidget::OnSetDefaultDataset()
{
    const QUuid id = CurrentInstanceId();
    const QString name = default_dataset_combo_->currentText();
    if (id.isNull())
    {
        return;
    }
    if (name.isEmpty())
    {
        status_label_->setText("No dataset to switch to (query state first).");
        return;
    }

    QJsonObject params;
    params["name"] = name;
    manager_->Call(id, QStringLiteral("dataset.set_default"), params,
                   [this, name](bool ok, const QJsonValue &, const QString &error) {
                       ReportResult(QString("Set default %1").arg(name), ok, error);
                   });
}

void LauncherWidget::OnQueryState()
{
    const QUuid id = CurrentInstanceId();
    if (id.isNull())
    {
        return;
    }

    manager_->Call(id, QStringLiteral("project.get_state"), {},
                   [this](bool ok, const QJsonValue &result, const QString &error) {
                       if (!ok)
                       {
                           ReportResult(QStringLiteral("Query"), false, error);
                           return;
                       }
                       const QJsonObject o = result.toObject();

                       default_dataset_combo_->clear();
                       for (const QJsonValue &v : o.value("datasets").toArray())
                       {
                           default_dataset_combo_->addItem(v.toString());
                       }
                       default_dataset_combo_->setEnabled(
                           default_dataset_combo_->count() > 0);
                       const QString default_ds =
                           o.value("default_dataset").toString();
                       const int index =
                           default_dataset_combo_->findText(default_ds);
                       if (index >= 0)
                       {
                           default_dataset_combo_->setCurrentIndex(index);
                       }

                       status_label_->setText(
                           QString("%1 | datasets: %2 | default: %3 | equations: %4")
                               .arg(o.value("title").toString())
                               .arg(o.value("datasets").toArray().size())
                               .arg(default_ds)
                               .arg(o.value("equations").toArray().size()));
                   });
}

} // namespace gui
} // namespace xresults
