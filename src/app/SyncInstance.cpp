#include "SyncInstance.h"
#include "MainWindow.h"
#include "tray/TrayController.h"
#include "health/HealthMonitor.h"

#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

SyncInstance::SyncInstance(InstanceConfig config,
                           SettingsManager *globalSettings,
                           QObject *parent)
    : QObject(parent), m_config(std::move(config)), m_globalSettings(globalSettings)
{
  // Window has no Qt parent so its lifecycle is fully controlled here.
  m_window = new MainWindow(m_globalSettings, m_config);
  m_tray = new TrayController(this);
  m_health = new HealthMonitor(this);

  m_tray->setLabel(m_config.name);
  m_health->setUrl(m_config.url);
  m_health->setIntervalMs(m_globalSettings->healthCheckIntervalMs());

  connect(m_health, &HealthMonitor::stateChanged,
          this, &SyncInstance::onHealthStateChanged);

  connect(m_tray, &TrayController::openWindowRequested, this, [this]()
          {
        m_window->show();
        m_window->raise();
        m_window->activateWindow(); });
  connect(m_tray, &TrayController::openInBrowserRequested, this, [this]()
          { QDesktopServices::openUrl(QUrl(m_config.url)); });
  connect(m_tray, &TrayController::refreshRequested, this, [this]()
          { m_health->setUrl(m_config.url); m_health->probeNow(); });
  connect(m_tray, &TrayController::reconnectRequested,
          this, &SyncInstance::reconnect);
  connect(m_tray, &TrayController::notificationClicked, this, [](const QString &url)
          { QDesktopServices::openUrl(QUrl(url)); });
  connect(m_tray, &TrayController::quitRequested,
          qApp, &QApplication::quit);

  connect(m_window, &MainWindow::instanceConfigChanged,
          this, &SyncInstance::onInstanceConfigChanged);
  connect(m_window, &MainWindow::manageInstancesRequested,
          this, &SyncInstance::manageInstancesRequested);
  connect(m_window, &MainWindow::reconnectRequested,
          this, &SyncInstance::reconnect);
  connect(m_window, &MainWindow::checkForUpdatesRequested,
          this, &SyncInstance::checkForUpdatesRequested);
}

SyncInstance::~SyncInstance()
{
  delete m_window;
}

void SyncInstance::applyConfig(const InstanceConfig &cfg)
{
  m_config = cfg;
  m_tray->setLabel(m_config.name);
  m_health->setUrl(m_config.url);
  m_window->applyConfig(cfg); // update title and reload webview
}

void SyncInstance::start(bool startMinimized)
{
  m_tray->show();
  if (!startMinimized)
    m_window->show();
  m_health->start();
}

void SyncInstance::showWindow()
{
  m_window->show();
  m_window->raise();
  m_window->activateWindow();
}

void SyncInstance::setThemeScript(const QString &source)
{
  m_window->setThemeScript(source);
}

void SyncInstance::reconnect()
{
  m_window->reconnect();
  m_health->probeNow();
}

void SyncInstance::notify(const QString &title, const QString &message,
                          const QString &url)
{
  m_tray->notify(title, message, url);
}

void SyncInstance::onHealthStateChanged(HealthState state)
{
  m_tray->updateState(state);
  m_window->applyHealthState(state);
  m_previousState = state;
}

void SyncInstance::onInstanceConfigChanged(const InstanceConfig &cfg)
{
  m_config = cfg;
  m_tray->setLabel(m_config.name);
  m_health->setUrl(m_config.url);

  // Persist the updated instance in the settings list.
  auto list = m_globalSettings->instances();
  for (auto &item : list)
  {
    if (item.id == cfg.id)
    {
      item = cfg;
      break;
    }
  }
  m_globalSettings->setInstances(list);
}
