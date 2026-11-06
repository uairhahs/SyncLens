#pragma once

#include <QApplication>
#include <QList>
#include <QPointer>
#include <QTimer>
#include "settings/SettingsManager.h"

class SyncInstance;
class UpdateChecker;
class ThemeManager;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application() override;

    int run();

private:
    void createInstances();
    SyncInstance *addInstance(const InstanceConfig &cfg);
    void applyInstanceChanges(const QList<InstanceConfig> &newList);
    void onManageInstancesRequested();

    void applyTheme(bool force = false);

    void scheduleUpdateCheck();
    void onCheckForUpdatesRequested(QWidget *parent);
    void onUpdateAvailable(const QString &version, const QString &url);
    void onUpToDate(const QString &latest);
    void onUpdateCheckFailed(const QString &error);

    SettingsManager *m_settings;
    QList<SyncInstance *> m_instances;
    bool m_manageDialogOpen = false;

    ThemeManager *m_themes;
    QString m_themeScript; // last script sent to the instances

    UpdateChecker *m_updateChecker;
    QTimer m_updateTimer;
    // Set while a check started from the Help menu is in flight, so its
    // result is shown in a dialog rather than as a notification.
    bool m_interactiveCheck = false;
    QPointer<QWidget> m_interactiveParent;
};
