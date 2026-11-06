#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QSize>
#include <QPoint>
#include <QList>
#include <QDateTime>

// Describes one Resilio Sync WebUI connection.
// The `id` is used as the QWebEngineProfile name, so profiles are isolated.
struct InstanceConfig
{
    QString id;   // persistent unique identifier (UUID string or "default")
    QString name; // user-visible label, e.g. "Home"
    QString url;  // WebUI URL, e.g. "http://127.0.0.1:8888/gui/"
};

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);

    // ---- Instance list ----
    // Returns at least one entry; migrates legacy single-URL config on first call.
    QList<InstanceConfig> instances() const;
    void setInstances(const QList<InstanceConfig> &list);

    // ---- Global application settings ----
    bool startMinimized() const;
    void setStartMinimized(bool value);

    bool launchOnLogin() const;
    void setLaunchOnLogin(bool value);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool value);

    int healthCheckIntervalMs() const;
    void setHealthCheckIntervalMs(int ms);

    // WebUI theme id (see ThemeManager); empty = Resilio's own colours.
    QString theme() const;
    void setTheme(const QString &id);

    bool checkForUpdates() const;
    void setCheckForUpdates(bool value);
    QDateTime lastUpdateCheck() const;
    void setLastUpdateCheck(const QDateTime &when);
    // Newest release already announced, so each one is notified only once.
    QString lastNotifiedVersion() const;
    void setLastNotifiedVersion(const QString &version);

    // Per-instance window geometry (keyed by instance id)
    QSize windowSize(const QString &instanceId) const;
    void setWindowSize(const QString &instanceId, const QSize &size);
    QPoint windowPos(const QString &instanceId) const;
    void setWindowPos(const QString &instanceId, const QPoint &pos);

signals:
    void themeChanged();

private:
    QSettings m_settings;
};
