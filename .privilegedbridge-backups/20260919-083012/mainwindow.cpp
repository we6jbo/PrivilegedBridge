#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

namespace {
constexpr const char *kTgCode = "TG239670";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadBuiltinActions();
    refreshActionList();

    ui->provenanceLabel->setText(QString("Project provenance: %1").arg(kTgCode));
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::onRunSelected);
    connect(ui->actionList, &QListWidget::currentRowChanged, this, &MainWindow::onSelectionChanged);

    if (!m_actions.isEmpty())
        ui->actionList->setCurrentRow(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadBuiltinActions()
{
    m_actions = {
        {"check_lxd_installed", "snap", {"list", "lxd"}, false,
         "Read-only check that reports whether the LXD snap is installed."},
        {"install_lxd", "snap", {"install", "lxd"}, true,
         "Install the LXD snap. Privileged; pkexec will request authorization."},
        {"initialize_lxd", "lxd", {"init", "--auto"}, true,
         "Initialize LXD with automatic defaults. Privileged; pkexec will request authorization."},
        {"add_user_to_lxd_group", "usermod", {"-a", "-G", "lxd", "__CURRENT_USER__"}, true,
         "Add the current login user to the lxd group. Log out/in afterward."},
        {"check_lxd_ready", "lxc", {"info"}, false,
         "Read-only readiness check for the current user's access to LXD."}
    };
}

void MainWindow::refreshActionList()
{
    ui->actionList->clear();
    for (const auto &action : m_actions)
        ui->actionList->addItem(action.name + (action.privileged ? "  [privileged]" : ""));
}

QStringList MainWindow::actionNames() const
{
    QStringList names;
    for (const auto &action : m_actions)
        names << action.name;
    return names;
}

const MainWindow::ActionDefinition *MainWindow::findAction(const QString &name) const
{
    for (const auto &action : m_actions) {
        if (action.name == name)
            return &action;
    }
    return nullptr;
}

QString MainWindow::effectiveUserName() const
{
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty())
        user = qEnvironmentVariable("LOGNAME");
    return user;
}

MainWindow::ActionResult MainWindow::executeAction(const ActionDefinition &action, bool interactiveConfirmation)
{
    ActionResult result;

    QString executable = QStandardPaths::findExecutable(action.executable);
    QStringList args = action.arguments;
    args.replaceInStrings("__CURRENT_USER__", effectiveUserName());

    if (executable.isEmpty()) {
        result.error = QString("Executable not found in PATH: %1\n").arg(action.executable);
        return result;
    }

    if (action.privileged) {
        if (interactiveConfirmation) {
            const auto answer = QMessageBox::question(
                this,
                "Privileged action",
                QString("Run privileged action '%1'?\n\n%2\n\nAuthorization will be requested by polkit/pkexec.")
                    .arg(action.name, action.description),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                result.error = "Action cancelled by user.\n";
                return result;
            }
        }

        const QString pkexec = QStandardPaths::findExecutable("pkexec");
        if (pkexec.isEmpty()) {
            result.error = "pkexec was not found. Install/configure polkit before privileged actions can run.\n";
            return result;
        }
        args.prepend(executable);
        executable = pkexec;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(executable, args);
    if (!process.waitForStarted(5000)) {
        result.error = QString("Failed to start: %1\n").arg(process.errorString());
        return result;
    }

    process.waitForFinished(-1);
    result.output = QString::fromUtf8(process.readAllStandardOutput());
    result.error = QString::fromUtf8(process.readAllStandardError());
    result.ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    return result;
}

MainWindow::ActionResult MainWindow::runNamedAction(const QString &name, bool interactiveConfirmation)
{
    const auto *action = findAction(name);
    if (!action)
        return {false, {}, QString("Unknown action: %1\n").arg(name)};
    return executeAction(*action, interactiveConfirmation);
}

QString MainWindow::statusJson() const
{
    QJsonObject root;
    root["project"] = "PrivilegedBridge";
    root["version"] = "0.1";
    root["tg_code"] = kTgCode;
    root["transport"] = "local-process-only";
    root["network_listener"] = false;

    QJsonArray actions;
    for (const auto &action : m_actions) {
        QJsonObject item;
        item["name"] = action.name;
        item["privileged"] = action.privileged;
        item["description"] = action.description;
        actions.append(item);
    }
    root["actions"] = actions;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void MainWindow::onSelectionChanged()
{
    const int row = ui->actionList->currentRow();
    if (row < 0 || row >= m_actions.size()) {
        ui->descriptionEdit->clear();
        return;
    }

    const auto &action = m_actions.at(row);
    ui->descriptionEdit->setPlainText(
        QString("Action: %1\nPrivileged: %2\nExecutable: %3\nArguments: %4\n\n%5")
            .arg(action.name,
                 action.privileged ? "yes" : "no",
                 action.executable,
                 action.arguments.join(' '),
                 action.description));
}

void MainWindow::onRunSelected()
{
    const int row = ui->actionList->currentRow();
    if (row < 0 || row >= m_actions.size())
        return;

    const auto result = executeAction(m_actions.at(row), true);
    QString text;
    text += result.ok ? "RESULT: SUCCESS\n\n" : "RESULT: FAILED\n\n";
    if (!result.output.isEmpty())
        text += "STDOUT:\n" + result.output + "\n";
    if (!result.error.isEmpty())
        text += "STDERR:\n" + result.error + "\n";
    ui->outputEdit->setPlainText(text);
}
