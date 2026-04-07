#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include "health/HealthState.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
QT_END_NAMESPACE

class TrayController : public QObject
{
    Q_OBJECT
public:
    explicit TrayController(QObject *parent = nullptr);

    void show();
    void updateState(HealthState state);

signals:
    void openWindowRequested();
    void openInBrowserRequested();
    void refreshRequested();
    void quitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void rebuildTooltip(HealthState state);

    QSystemTrayIcon *m_tray;
    QMenu           *m_menu;
    QAction         *m_openAction;
    QAction         *m_openBrowserAction;
    QAction         *m_refreshAction;
    QAction         *m_quitAction;
};
