#pragma once

#include <QObject>
#include <QList>
#include <QSystemTrayIcon>
#include "health/HealthState.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
QT_END_NAMESPACE

// The single application-wide tray icon. It shows the worst health state of
// all instances; with several instances the menu lists them, and choosing
// one opens its window.
class TrayController : public QObject
{
    Q_OBJECT
public:
    struct Entry
    {
        QString id;
        QString name;
        HealthState state = HealthState::Unknown;
    };

    explicit TrayController(QObject *parent = nullptr);
    ~TrayController() override;

    void show();
    // Replace the instance list (order = menu order).
    void setInstances(const QList<Entry> &entries);
    // Instance whose window a left click opens.
    void setDefaultInstance(const QString &id);
    void updateState(const QString &id, HealthState state);
    // Desktop notification from the tray icon; clicking it opens `url`.
    void notify(const QString &title, const QString &message, const QString &url);

signals:
    void openWindowRequested(const QString &id);
    void refreshRequested(const QString &id);
    void reconnectRequested(const QString &id);
    void notificationClicked(const QString &url);
    void quitRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void rebuildMenu();
    void refresh(); // icon + tooltip + menu labels
    void addInstanceActions(QMenu *menu, const QString &id);
    HealthState aggregateState() const;

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    QList<Entry> m_entries;
    QList<QAction *> m_instanceActions; // parallel to m_entries when there are several
    QString m_defaultId;
    QString m_notifyUrl;
};
