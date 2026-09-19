#include "actionrunner.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {
constexpr const char *kTgCode = "TG239670";

bool commandOk(const ActionRunner::ActionResult &r)
{
    return r.ok;
}
}

ActionRunner::ActionRunner()
{
    loadBuiltinActions();
}

void ActionRunner::loadBuiltinActions()
{
    // Deliberately fixed allowlist. No action accepts arbitrary commands or arguments.
    m_actions = {
        {"check_snapd_socket", "systemctl", {"is-active", "snapd.socket"}, false,
         "Read-only check that snapd socket activation is running."},
        {"check_snapd_apparmor", "systemctl", {"is-active", "snapd.apparmor.service"}, false,
         "Read-only check for snapd's AppArmor profile loader."},
        {"check_snap_confinement", "snap", {"debug", "confinement"}, false,
         "Read-only report of snapd confinement support on this host."},
        {"check_snap_changes", "snap", {"changes"}, false,
         "Read-only list of recent snapd operations and failures."},
        {"check_snapcraft_installed", "snap", {"list", "snapcraft"}, false,
         "Read-only check that Snapcraft is installed."},
        {"install_snapcraft", "snap", {"install", "snapcraft", "--classic"}, true,
         "Install Snapcraft from the Snap Store using classic confinement."},

        {"check_lxd_installed", "snap", {"list", "lxd"}, false,
         "Read-only check that the LXD snap is installed."},
        {"install_lxd", "snap", {"install", "lxd"}, true,
         "Install the LXD snap. pkexec/polkit handles authorization."},
        {"initialize_lxd", "lxd", {"init", "--auto"}, true,
         "Initialize LXD with automatic defaults. pkexec/polkit handles authorization."},
        {"add_user_to_lxd_group", "usermod", {"-a", "-G", "lxd", "__CURRENT_USER__"}, true,
         "Add the current login user to the lxd group. A logout/login is required afterward."},
        {"check_lxd_ready", "lxc", {"info"}, false,
         "Read-only readiness check for the current user's access to LXD."},
        {"check_lxd_activation_log", "journalctl", {"-u", "snap.lxd.activate.service", "-b", "--no-pager", "-n", "120"}, false,
         "Read-only recent log for the LXD snap activation service."},

        {"check_multipass_installed", "snap", {"list", "multipass"}, false,
         "Read-only check that Multipass is installed."},
        {"install_multipass", "snap", {"install", "multipass"}, true,
         "Install Multipass as an alternative Snapcraft build provider."},
        {"check_multipass_ready", "multipass", {"version"}, false,
         "Read-only check that the Multipass client can communicate with its service."},
        {"check_multipass_instances", "multipass", {"list", "--format", "json"}, false,
         "Read-only list of Multipass instances in JSON form."},

        {"check_kvm_device", "stat", {"/dev/kvm"}, false,
         "Read-only check for the KVM device used for hardware-assisted virtualization."},
        {"check_snapcraft_provider", "@internal:provider_status", {}, false,
         "Return structured readiness information for LXD and Multipass and identify usable Snapcraft providers."},

        {"build_snap_default", "snapcraft", {}, false,
         "Build the current Snapcraft project using Snapcraft's default provider."},
        {"build_snap_with_multipass", "snapcraft", {}, false,
         "Build the current Snapcraft project with Multipass forced as the provider.",
         {{"SNAPCRAFT_BUILD_ENVIRONMENT", "multipass"}}}
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

QString ActionRunner::resolveExecutable(const QString &name) const
{
    if (name.startsWith('/'))
        return QFileInfo::exists(name) ? name : QString();

    QString executable = QStandardPaths::findExecutable(name);
    if (!executable.isEmpty())
        return executable;

    // Snap commands may exist even when the current shell has not yet picked up /snap/bin.
    const QStringList snapBins = {
        "/snap/bin/" + name,
        "/var/lib/snapd/snap/bin/" + name
    };
    for (const QString &candidate : snapBins) {
        QFileInfo fi(candidate);
        if (fi.exists() && fi.isExecutable())
            return candidate;
    }
    return {};
}

ActionRunner::ActionResult ActionRunner::runProcess(const QString &actionName,
                                                     const QString &executableName,
                                                     const QStringList &arguments,
                                                     const QMap<QString, QString> &environment) const
{
    ActionResult result;
    result.action = actionName;

    const QString executable = resolveExecutable(executableName);
    if (executable.isEmpty()) {
        result.error = QString("Executable not found: %1\n").arg(executableName);
        return result;
    }

    result.executable = executable;
    result.arguments = arguments;

    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (auto it = environment.cbegin(); it != environment.cend(); ++it)
        env.insert(it.key(), it.value());
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(executable, arguments);
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

ActionRunner::ActionResult ActionRunner::executeInternal(const ActionDefinition &action) const
{
    ActionResult result;
    result.action = action.name;
    result.executable = action.executable;

    if (action.executable == "@internal:provider_status") {
        const auto snapd = runProcess(action.name, "systemctl", {"is-active", "snapd.socket"});
        const auto confinement = runProcess(action.name, "snap", {"debug", "confinement"});
        const auto lxdInstalled = runProcess(action.name, "snap", {"list", "lxd"});
        const auto lxdReady = runProcess(action.name, "lxc", {"info"});
        const auto mpInstalled = runProcess(action.name, "snap", {"list", "multipass"});
        const auto mpReady = runProcess(action.name, "multipass", {"version"});

        QJsonObject root;
        root["snapd_socket_active"] = commandOk(snapd);
        root["snap_confinement"] = confinement.output.trimmed();
        root["lxd_installed"] = commandOk(lxdInstalled);
        root["lxd_ready"] = commandOk(lxdReady);
        root["multipass_installed"] = commandOk(mpInstalled);
        root["multipass_ready"] = commandOk(mpReady);
        root["kvm_device_present"] = QFileInfo::exists("/dev/kvm");

        QJsonArray usable;
        if (commandOk(lxdReady))
            usable.append("lxd");
        if (commandOk(mpReady))
            usable.append("multipass");
        root["usable_providers"] = usable;

        if (commandOk(lxdReady))
            root["suggested_provider"] = "lxd";
        else if (commandOk(mpReady))
            root["suggested_provider"] = "multipass";
        else if (commandOk(mpInstalled))
            root["suggested_provider"] = "multipass-needs-attention";
        else
            root["suggested_provider"] = "none-ready";

        root["note"] = "Multipass can be forced for a build by setting SNAPCRAFT_BUILD_ENVIRONMENT=multipass.";
        result.output = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)) + "\n";
        result.ok = commandOk(snapd) && (!usable.isEmpty());
        result.exitCode = result.ok ? 0 : 1;
        return result;
    }

    result.error = "Unknown internal action.\n";
    return result;
}

ActionRunner::ActionResult ActionRunner::executeAction(const ActionDefinition &action) const
{
    if (action.executable.startsWith("@internal:"))
        return executeInternal(action);

    QString executable = resolveExecutable(action.executable);
    QStringList args = action.arguments;
    args.replaceInStrings("__CURRENT_USER__", effectiveUserName());

    ActionResult result;
    result.action = action.name;
    result.arguments = args;

    if (executable.isEmpty()) {
        result.error = QString("Executable not found: %1\n").arg(action.executable);
        return result;
    }

    if (!action.privileged)
        return runProcess(action.name, executable, args, action.environment);

    const QString pkexec = resolveExecutable("pkexec");
    if (pkexec.isEmpty()) {
        result.error = "pkexec was not found. Install/configure polkit before privileged actions can run.\n";
        return result;
    }

    // pkexec receives only the fixed executable and fixed allowlisted arguments.
    QStringList privilegedArgs;
    privilegedArgs << executable;
    privilegedArgs << args;
    return runProcess(action.name, pkexec, privilegedArgs, action.environment);
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
    root["snap_bin_fallback"] = true;

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
    QJsonObject env;
    for (auto it = action->environment.cbegin(); it != action->environment.cend(); ++it)
        env[it.key()] = it.value();
    root["environment"] = env;
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
