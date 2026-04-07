#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QSize>
#include <QPoint>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString webUiUrl() const;
    void setWebUiUrl(const QString &url);

    bool startMinimized() const;
    void setStartMinimized(bool value);

    bool launchOnLogin() const;
    void setLaunchOnLogin(bool value);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool value);

    int healthCheckIntervalMs() const;
    void setHealthCheckIntervalMs(int ms);

    QSize windowSize() const;
    void setWindowSize(const QSize &size);

    QPoint windowPos() const;
    void setWindowPos(const QPoint &pos);

private:
    QSettings m_settings;
};
