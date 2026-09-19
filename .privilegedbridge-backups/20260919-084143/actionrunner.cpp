#include "actionrunner.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace {
constexpr const char *kTgCode = "TG239670";
}

ActionRunner::ActionRunner()
{
    loadBuiltinActions();
}

void ActionRunner::loadBuiltinActions()
{
    // Deliberately fixed allowlist. No action accepts arbitrary commands or arguments.
    m_actions = {
        {"check_lxd_installed", "snap", {"list", "lxd"}, false,
         "Read-only check that reports whether the LXD snap is installed."},
        {"install_lxd", "snap", {"install", "lxd"}, true,
         "Install the LXD snap. pkexec/polkit handles authorization."},
        {"initialize_lxd", "lxd", {"init", "--auto"}, true,
         "Initialize LXD with automatic defaults. pkexec/polkit handles authorization."},
        {"add_user_to_lxd_group", "usermod", {"-a", "-G", "lxd", "__CURRENT_USER__"}, true,
         "Add the current login user to the lxd group. A logout/login is required afterward."},
        {"check_lxd_ready", "lxc", {"info"}, false,
         "Read-only readiness check for the current user's access to LXD."}
    };
}

QStringList ActionRunner::actionNames() const
{
    QStringList names;
    for (const auto &action : m_actions)
        names << action.name;
    return names;
}

const ActionRunner::ActionDefinition *ActionRunner::findAction(const QString &name) const
{
    for (const auto &action : m_actions) {
        if (action.name == name)
            return &action;
    }
    return nullptr;
}

QString ActionRunner::effectiveUserName() const
{
    QString user = qEnvironmentVariable("SUDO_USER");
    if (user.isEmpty())
        user = qEnvironmentVariable("USER");
    if (user.isEmpty())
        user = qEnvironmentVariable("LOGNAME");
    return user;
}

ActionRunner::ActionResult ActionRunner::executeAction(const ActionDefinition &action) const
{
    ActionResult result;
    result.action = action.name;

    QString executable = QStandardPaths::findExecutable(action.executable);
    QStringList args = action.arguments;
    args.replaceInStrings("__CURRENT_USER__", effectiveUserName());

    if (executable.isEmpty()) {
        result.error = QString("Executable not found in PATH: %1\n").arg(action.executable);
        return result;
    }

    result.executable = executable;
    result.arguments = args;

    if (action.privileged) {
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
    result.exitCode = process.exitCode();
    result.ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    return result;
}

ActionRunner::ActionResult ActionRunner::runNamedAction(const QString &name) const
{
    const auto *action = findAction(name);
    if (!action) {
        ActionResult result;
        result.action = name;
        result.error = QString("Unknown action: %1\n").arg(name);
        return result;
    }
    return executeAction(*action);
}

QString ActionRunner::statusJson() const
{
    QJsonObject root;
    root["project"] = "PrivilegedBridge";
    root["version"] = QCoreApplication::applicationVersion();
    root["tg_code"] = kTgCode;
    root["interface"] = "command-line";
    root["transport"] = "local-process-only";
    root["network_listener"] = false;
    root["arbitrary_shell"] = false;

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

QString ActionRunner::describeJson(const QString &name) const
{
    const auto *action = findAction(name);
    QJsonObject root;
    root["name"] = name;
    if (!action) {
        root["known"] = false;
        return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    }

    root["known"] = true;
    root["privileged"] = action->privileged;
    root["executable"] = action->executable;
    root["description"] = action->description;
    QJsonArray args;
    for (const auto &arg : action->arguments)
        args.append(arg);
    root["arguments"] = args;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString ActionRunner::resultJson(const ActionResult &result) const
{
    QJsonObject root;
    root["ok"] = result.ok;
    root["action"] = result.action;
    root["exit_code"] = result.exitCode;
    root["stdout"] = result.output;
    root["stderr"] = result.error;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}
