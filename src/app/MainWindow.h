#pragma once

#include <QMainWindow>
#include "health/HealthState.h"

class WebViewHost;
class SettingsManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(SettingsManager *settings, QWidget *parent = nullptr);

    void applyHealthState(HealthState state);

public slots:
    void openInBrowser();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    void setupMenuBar();

    WebViewHost     *m_webViewHost;
    SettingsManager *m_settings;
    HealthState      m_lastState = HealthState::Unknown;
    bool             m_webViewLoaded = false;
};
