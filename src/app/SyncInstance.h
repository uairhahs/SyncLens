#pragma once

#include <QObject>
#include "health/HealthState.h"
#include "settings/SettingsManager.h"

class MainWindow;
class HealthMonitor;

// Owns the per-instance window and health monitor. One SyncInstance exists
// for each configured Resilio Sync endpoint; the tray icon is shared and
// owned by Application.
class SyncInstance : public QObject
{
  Q_OBJECT
public:
  SyncInstance(InstanceConfig config,
               SettingsManager *globalSettings,
               QObject *parent = nullptr);
  ~SyncInstance() override;

  const InstanceConfig &config() const { return m_config; }
  HealthState healthState() const { return m_state; }

  // Apply a changed config at runtime (URL, name). Caller is responsible
  // for persisting the new list via SettingsManager::setInstances().
  void applyConfig(const InstanceConfig &cfg);

  // Start health polling. The window is shown separately (one at a time).
  void start();

  void showWindow();
  void hideWindow(); // close to tray, as the window's close button does
  void refreshStatus();
  void reconnect();
  void setThemeScript(const QString &source);

signals:
  void healthStateChanged(const QString &id, HealthState state);
  // Name changed via the window's Settings dialog.
  void configChanged(const InstanceConfig &cfg);
  void manageInstancesRequested();
  void checkForUpdatesRequested(QWidget *parent);

private slots:
  void onHealthStateChanged(HealthState state);
  void onInstanceConfigChanged(const InstanceConfig &cfg);

private:
  InstanceConfig m_config;
  SettingsManager *m_globalSettings;
  MainWindow *m_window; // no Qt parent; deleted in destructor
  HealthMonitor *m_health;
  HealthState m_state = HealthState::Unknown;
};
