#pragma once

#include <QApplication>
#include <QList>
#include "settings/SettingsManager.h"

class SyncInstance;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application() override;

    int run();

private:
    void createInstances();
    void applyInstanceChanges(const QList<InstanceConfig> &newList);
    void onManageInstancesRequested();

    SettingsManager *m_settings;
    QList<SyncInstance *> m_instances;
    bool m_manageDialogOpen = false;
};
