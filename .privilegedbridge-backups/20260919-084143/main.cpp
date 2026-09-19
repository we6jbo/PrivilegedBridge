#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include "actionrunner.h"

namespace {
constexpr const char *kProjectId = "PrivilegedBridge";
constexpr const char *kProjectRoot = "/home/we6jbo/Projects/PrivilegedBridge";
constexpr const char *kTgCode = "TG239670";

void attemptTgRegistration()
{
    const QString helper = QStandardPaths::findExecutable("tg-register-project");
    if (helper.isEmpty())
        return;

    QProcess p;
    p.start(helper, {
        "--project-id", kProjectId,
        "--project-root", kProjectRoot,
        "--codes", kTgCode,
        "--reference", "PrivilegedBridge CLI helper"
    });
    p.waitForFinished(2500);
}

void printUsage(QTextStream &out)
{
    out << "PrivilegedBridge 0.2 - local allowlisted privileged-command bridge\n\n"
        << "Usage:\n"
        << "  privileged-bridge --list\n"
        << "  privileged-bridge --status\n"
        << "  privileged-bridge --describe ACTION\n"
        << "  privileged-bridge --run ACTION\n"
        << "  privileged-bridge --run-json ACTION\n\n"
        << "No GUI, network listener, or arbitrary shell interface is provided.\n";
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("PrivilegedBridge");
    QCoreApplication::setApplicationVersion("0.2");

    attemptTgRegistration();

    QTextStream out(stdout);
    QTextStream err(stderr);
    ActionRunner runner;
    const QStringList args = QCoreApplication::arguments();

    if (args.size() == 1 || args.at(1) == "--help" || args.at(1) == "-h") {
        printUsage(out);
        return 0;
    }

    if (args.at(1) == "--list") {
        for (const QString &name : runner.actionNames())
            out << name << '\n';
        return 0;
    }

    if (args.at(1) == "--status") {
        out << runner.statusJson() << '\n';
        return 0;
    }

    if (args.at(1) == "--describe" && args.size() >= 3) {
        out << runner.describeJson(args.at(2)) << '\n';
        return 0;
    }

    if ((args.at(1) == "--run" || args.at(1) == "--run-json") && args.size() >= 3) {
        const auto result = runner.runNamedAction(args.at(2));
        if (args.at(1) == "--run-json") {
            out << runner.resultJson(result) << '\n';
        } else {
            if (!result.output.isEmpty())
                out << result.output;
            if (!result.error.isEmpty())
                err << result.error;
        }
        return result.ok ? 0 : 1;
    }

    err << "Invalid arguments.\n\n";
    printUsage(err);
    return 2;
}
