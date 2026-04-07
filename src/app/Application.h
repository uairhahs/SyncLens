#pragma once

#include <QApplication>
#include "health/HealthState.h"

class SettingsManager;
class MainWindow;
class TrayController;
class HealthMonitor;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application() override;

    int run();

private:
    void onHealthStateChanged(HealthState state);
    void onOpenInBrowserRequested();

    SettingsManager *m_settings;
    MainWindow      *m_mainWindow;
    TrayController  *m_tray;
    HealthMonitor   *m_health;

    HealthState m_previousHealthState;
    bool        m_firstRun = true;
};
