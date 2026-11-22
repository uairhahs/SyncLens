#include "SyncInstance.h"
#include "MainWindow.h"
#include "health/HealthMonitor.h"


SyncInstance::SyncInstance(InstanceConfig config,
                           SettingsManager *globalSettings,
                           QObject *parent)
    : QObject(parent), m_config(std::move(config)), m_globalSettings(globalSettings)
{
  // Window has no Qt parent so its lifecycle is fully controlled here.
  m_window = new MainWindow(m_globalSettings, m_config);
  m_health = new HealthMonitor(this);

  m_health->setUrl(m_config.url);
  m_health->setIntervalMs(m_globalSettings->healthCheckIntervalMs());

  connect(m_health, &HealthMonitor::stateChanged,
          this, &SyncInstance::onHealthStateChanged);

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
  m_health->setUrl(m_config.url);
  m_window->applyConfig(cfg); // update title and reload webview
}

void SyncInstance::start()
{
  m_health->start();
}

void SyncInstance::showWindow()
{
  // show() alone leaves a minimised window in the taskbar.
  m_window->setWindowState(m_window->windowState() & ~Qt::WindowMinimized);
  m_window->show();
  m_window->raise();
  m_window->activateWindow();
}

void SyncInstance::hideWindow()
{
  m_window->hide();
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

void SyncInstance::refreshStatus()
{
  m_health->setUrl(m_config.url);
  m_health->probeNow();
}

void SyncInstance::onHealthStateChanged(HealthState state)
{
  m_state = state;
  m_window->applyHealthState(state);
  emit healthStateChanged(m_config.id, state);
}

void SyncInstance::onInstanceConfigChanged(const InstanceConfig &cfg)
{
  m_config = cfg;
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
  emit configChanged(m_config);
}
