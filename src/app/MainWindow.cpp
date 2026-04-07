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

MainWindow::MainWindow(SettingsManager *settings, QWidget *parent)
    : QMainWindow(parent)
    , m_settings(settings)
{
    setWindowTitle("SyncLens");
    resize(m_settings->windowSize());
    move(m_settings->windowPos());

    m_webViewHost = new WebViewHost(this);
    setCentralWidget(m_webViewHost);
    m_webViewHost->setUrl(m_settings->webUiUrl());

    connect(m_webViewHost, &WebViewHost::openInBrowserRequested,
            this, [](const QString &url) {
        QDesktopServices::openUrl(QUrl(url));
    });

    setupMenuBar();
    m_webViewHost->showOfflinePage(tr("Connecting to Resilio Sync..."));
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *settingsAction = fileMenu->addAction(tr("&Settings..."));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        SettingsDialog dlg(m_settings, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_webViewHost->setUrl(m_settings->webUiUrl());
            m_webViewHost->loadUrl(m_settings->webUiUrl());
        }
    });

    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = helpMenu->addAction(tr("&About SyncLens"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        AboutDialog dlg(this);
        dlg.exec();
    });
}

void MainWindow::applyHealthState(HealthState state)
{
    if (m_lastState == state)
        return;
    m_lastState = state;

    if (state == HealthState::Running) {
        if (!m_webViewLoaded) {
            m_webViewLoaded = true;
            m_webViewHost->loadUrl(m_settings->webUiUrl());
        }
        m_webViewHost->showWebView();
    } else {
        QString msg;
        switch (state) {
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
    QDesktopServices::openUrl(QUrl(m_settings->webUiUrl()));
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
    m_settings->setWindowSize(size());
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    m_settings->setWindowPos(pos());
}
