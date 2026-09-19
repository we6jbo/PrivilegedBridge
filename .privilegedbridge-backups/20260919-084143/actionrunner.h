#pragma once

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
    QString effectiveUserName() const;
};
