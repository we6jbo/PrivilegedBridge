#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct ActionResult {
        bool ok = false;
        QString output;
        QString error;
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QStringList actionNames() const;
    QString statusJson() const;
    ActionResult runNamedAction(const QString &name, bool interactiveConfirmation = true);

private slots:
    void onRunSelected();
    void onSelectionChanged();

private:
    struct ActionDefinition {
        QString name;
        QString executable;
        QStringList arguments;
        bool privileged = false;
        QString description;
    };

    Ui::MainWindow *ui;
    QVector<ActionDefinition> m_actions;

    void loadBuiltinActions();
    void refreshActionList();
    const ActionDefinition *findAction(const QString &name) const;
    ActionResult executeAction(const ActionDefinition &action, bool interactiveConfirmation);
    QString effectiveUserName() const;
};
