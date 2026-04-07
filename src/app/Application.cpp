#include "Application.h"
#include "MainWindow.h"
#include "settings/SettingsManager.h"
#include "tray/TrayController.h"
#include "health/HealthMonitor.h"

#include <QSystemTrayIcon>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_previousHealthState(HealthState::Unknown)
{
    setApplicationName("SyncLens");
    setOrganizationName("SyncLens");
    setQuitOnLastWindowClosed(false);

    m_settings   = new SettingsManager(this);
    m_mainWindow = new MainWindow(m_settings);
    m_tray       = new TrayController(this);
    m_health     = new HealthMonitor(this);

    m_health->setUrl(m_settings->webUiUrl());
    m_health->setIntervalMs(m_settings->healthCheckIntervalMs());

    // Health -> Tray + Window
    connect(m_health, &HealthMonitor::stateChanged,
            this, [this](HealthState state) { onHealthStateChanged(state); });

    // Tray actions
    connect(m_tray, &TrayController::openWindowRequested, this, [this]() {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    });
    connect(m_tray, &TrayController::openInBrowserRequested,
            this, &Application::onOpenInBrowserRequested);
    connect(m_tray, &TrayController::refreshRequested, this, [this]() {
        // Force a fresh URL in case settings changed
        m_health->setUrl(m_settings->webUiUrl());
    });
    connect(m_tray, &TrayController::quitRequested,
            this, &QApplication::quit);
}

Application::~Application() = default;

int Application::run()
{
    m_tray->show();

    if (!m_settings->startMinimized()) {
        m_mainWindow->show();
    }

    m_health->start();
    return exec();
}

void Application::onHealthStateChanged(HealthState state)
{
    bool wasHealthy = (m_previousHealthState == HealthState::Running);
    bool isHealthy  = (state == HealthState::Running);

    m_tray->updateState(state);
    m_mainWindow->applyHealthState(state);

    m_previousHealthState = state;
    m_firstRun = false;

    Q_UNUSED(wasHealthy)
    Q_UNUSED(isHealthy)
}

void Application::onOpenInBrowserRequested()
{
    m_mainWindow->openInBrowser();
}
