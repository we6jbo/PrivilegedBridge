#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class ActionRunner
{
public:
    struct ActionDefinition {
        QString name;
        QString executable;
        QStringList arguments;
        bool privileged = false;
        QString description;
        QMap<QString, QString> environment;
    };

    struct ActionResult {
        bool ok = false;
        int exitCode = -1;
        QString action;
        QString executable;
        QStringList arguments;
        QString output;
        QString error;
    };

    ActionRunner();

    QStringList actionNames() const;
    QString statusJson() const;
    QString describeJson(const QString &name) const;
    ActionResult runNamedAction(const QString &name) const;
    QString resultJson(const ActionResult &result) const;

private:
    QVector<ActionDefinition> m_actions;

    void loadBuiltinActions();
    const ActionDefinition *findAction(const QString &name) const;
    ActionResult executeAction(const ActionDefinition &action) const;
    ActionResult executeInternal(const ActionDefinition &action) const;
    QString effectiveUserName() const;
    QString resolveExecutable(const QString &name) const;
    ActionResult runProcess(const QString &actionName,
                            const QString &executable,
                            const QStringList &arguments,
                            const QMap<QString, QString> &environment = {}) const;
};
