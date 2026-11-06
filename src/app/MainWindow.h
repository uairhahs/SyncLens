#pragma once

#include <QMainWindow>
#include "health/HealthState.h"
#include "settings/SettingsManager.h"

class WebViewHost;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(SettingsManager *settings, const InstanceConfig &config,
               QWidget *parent = nullptr);

    void applyHealthState(HealthState state);
    void applyConfig(const InstanceConfig &cfg);
    void reconnect();
    void setThemeScript(const QString &source);

public slots:
    void openInBrowser();

signals:
    void instanceConfigChanged(const InstanceConfig &config);
    void manageInstancesRequested();
    void reconnectRequested();
    void checkForUpdatesRequested(QWidget *parent);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    void setupMenuBar();
    void updateWindowTitle();

    WebViewHost *m_webViewHost;
    SettingsManager *m_settings;
    InstanceConfig m_config;
    HealthState m_lastState = HealthState::Unknown;
    bool m_webViewLoaded = false;
};
