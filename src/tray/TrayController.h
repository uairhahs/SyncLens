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
    // Sets the instance label shown in the tooltip (e.g. "Home").
    // Leave empty for a single-instance setup.
    void setLabel(const QString &label);
    // Desktop notification from this tray icon; clicking it opens `url`.
    void notify(const QString &title, const QString &message, const QString &url);

signals:
    void openWindowRequested();
    void openInBrowserRequested();
    void refreshRequested();
    void reconnectRequested();
    void notificationClicked(const QString &url);
    void quitRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void rebuildTooltip(HealthState state);
    void applyIcon();

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    QAction *m_openAction;
    QAction *m_openBrowserAction;
    QAction *m_refreshAction;
    QAction *m_reconnectAction;
    QAction *m_quitAction;
    QString m_label;
    QString m_notifyUrl;
    HealthState m_state = HealthState::Unknown;
};
