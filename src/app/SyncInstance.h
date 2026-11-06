#pragma once

#include <QObject>
#include "health/HealthState.h"
#include "settings/SettingsManager.h"

class MainWindow;
class TrayController;
class HealthMonitor;

// Owns the complete per-instance trio: window, tray icon, and health monitor.
// One SyncInstance exists for each configured Resilio Sync endpoint.
class SyncInstance : public QObject
{
  Q_OBJECT
public:
  SyncInstance(InstanceConfig config,
               SettingsManager *globalSettings,
               QObject *parent = nullptr);
  ~SyncInstance() override;

  const InstanceConfig &config() const { return m_config; }

  // Apply a changed config at runtime (URL, name). Caller is responsible
  // for persisting the new list via SettingsManager::setInstances().
  void applyConfig(const InstanceConfig &cfg);

  // Show tray + optionally open window, then start health polling.
  void start(bool startMinimized);

  void showWindow();
  void reconnect();
  void notify(const QString &title, const QString &message, const QString &url);

signals:
  void manageInstancesRequested();
  void checkForUpdatesRequested(QWidget *parent);

private slots:
  void onHealthStateChanged(HealthState state);
  void onInstanceConfigChanged(const InstanceConfig &cfg);

private:
  InstanceConfig m_config;
  SettingsManager *m_globalSettings;
  MainWindow *m_window; // no Qt parent — deleted in destructor
  TrayController *m_tray;
  HealthMonitor *m_health;
  HealthState m_previousState = HealthState::Unknown;
};
