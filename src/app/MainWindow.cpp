#include "MainWindow.h"
#include "webview/WebViewHost.h"
#include "settings/SettingsManager.h"
#include "ui/AboutDialog.h"
#include "ui/SettingsDialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QKeySequence>

MainWindow::MainWindow(SettingsManager *settings, const InstanceConfig &config,
                       QWidget *parent)
    : QMainWindow(parent), m_settings(settings), m_config(config)
{
    updateWindowTitle();
    resize(m_settings->windowSize(m_config.id));
    move(m_settings->windowPos(m_config.id));

    m_webViewHost = new WebViewHost(m_config.id, this);
    setCentralWidget(m_webViewHost);
    m_webViewHost->setUrl(m_config.url);

    connect(m_webViewHost, &WebViewHost::openInBrowserRequested,
            this, [](const QString &url)
            { QDesktopServices::openUrl(QUrl(url)); });

    setupMenuBar();
    m_webViewHost->showOfflinePage(tr("Connecting to Resilio Sync..."));
}

void MainWindow::updateWindowTitle()
{
    const bool isDefault = m_config.name.isEmpty() || m_config.name == QLatin1String("Default");
    setWindowTitle(isDefault ? QStringLiteral("SyncLens")
                             : QStringLiteral("SyncLens \u2014 ") + m_config.name);
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *settingsAction = fileMenu->addAction(tr("&Settings..."));
    connect(settingsAction, &QAction::triggered, this, [this]()
            {
        SettingsDialog dlg(m_settings, &m_config, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_webViewHost->setUrl(m_config.url);
            m_webViewHost->loadUrl(m_config.url);
            updateWindowTitle();
            emit instanceConfigChanged(m_config);
        } });

    auto *manageAction = fileMenu->addAction(tr("&Manage Instances..."));
    connect(manageAction, &QAction::triggered,
            this, &MainWindow::manageInstancesRequested);

    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    auto *reconnectAction = viewMenu->addAction(tr("&Reconnect"));
    reconnectAction->setShortcut(QKeySequence::Refresh);
    connect(reconnectAction, &QAction::triggered,
            this, &MainWindow::reconnectRequested);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *updatesAction = helpMenu->addAction(tr("Check for &Updates..."));
    connect(updatesAction, &QAction::triggered, this, [this]()
            { emit checkForUpdatesRequested(this); });
    auto *aboutAction = helpMenu->addAction(tr("&About SyncLens"));
    connect(aboutAction, &QAction::triggered, this, [this]()
            {
        AboutDialog dlg(this);
        dlg.exec(); });
}

void MainWindow::applyConfig(const InstanceConfig &cfg)
{
    m_config = cfg;
    updateWindowTitle();
    m_webViewHost->setUrl(m_config.url);
    m_webViewHost->loadUrl(m_config.url);
    m_webViewLoaded = true;
    // Reset health display so the new URL is shown if health state hasn't changed.
    m_lastState = HealthState::Unknown;
}

void MainWindow::setThemeScript(const QString &source)
{
    m_webViewHost->setThemeScript(source);
}

void MainWindow::reconnect()
{
    m_webViewLoaded = true;
    m_webViewHost->reconnect();
}

void MainWindow::applyHealthState(HealthState state)
{
    if (m_lastState == state)
        return;
    const HealthState previous = m_lastState;
    m_lastState = state;

    if (state == HealthState::Running)
    {
        if (!m_webViewLoaded)
        {
            m_webViewLoaded = true;
            m_webViewHost->loadUrl(m_config.url);
        }
        else if (previous == HealthState::Stopped ||
                 previous == HealthState::Unreachable ||
                 previous == HealthState::Error)
        {
            // The WebUI came back after an outage: the page still shows the
            // failed load (or a dead session), so start it afresh.
            m_webViewHost->reconnect();
        }
        m_webViewHost->showWebView();
    }
    else
    {
        QString msg;
        switch (state)
        {
        case HealthState::Stopped:
            msg = tr("The Resilio Sync daemon does not appear to be running.\n"
                     "Start the service and SyncLens will reconnect automatically.");
            break;
        case HealthState::Unreachable:
            msg = tr("The Resilio Sync WebUI is not responding.\n"
                     "Check that the daemon is running and the URL is correct.");
            break;
        case HealthState::Error:
            msg = tr("SyncLens encountered an error.\n"
                     "Check your settings and try again.");
            break;
        default:
            msg = tr("Connecting to Resilio Sync...");
            break;
        }
        m_webViewHost->showOfflinePage(msg);
    }
}

void MainWindow::openInBrowser()
{
    QDesktopServices::openUrl(QUrl(m_config.url));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Hide to tray instead of quitting.
    hide();
    event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_settings->setWindowSize(m_config.id, size());
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    m_settings->setWindowPos(m_config.id, pos());
}
