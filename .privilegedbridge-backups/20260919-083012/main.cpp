#include <QApplication>
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include "mainwindow.h"

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
        "--reference", "PrivilegedBridge generated project"
    });
    p.waitForFinished(2500);
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("PrivilegedBridge");
    QCoreApplication::setApplicationVersion("0.1");

    attemptTgRegistration();

    MainWindow window;

    const QStringList args = QCoreApplication::arguments();
    if (args.size() >= 2) {
        QTextStream out(stdout);
        QTextStream err(stderr);

        if (args.at(1) == "--list") {
            for (const QString &name : window.actionNames())
                out << name << '\n';
            return 0;
        }

        if (args.at(1) == "--status") {
            out << window.statusJson() << '\n';
            return 0;
        }

        if (args.at(1) == "--run" && args.size() >= 3) {
            const auto result = window.runNamedAction(args.at(2), false);
            out << result.output;
            if (!result.error.isEmpty())
                err << result.error;
            return result.ok ? 0 : 1;
        }
    }

    window.show();
    return app.exec();
}
