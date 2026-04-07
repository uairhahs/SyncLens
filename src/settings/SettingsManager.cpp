#include "SettingsManager.h"

static constexpr const char *kDefaultUrl = "http://127.0.0.1:8888/gui/";
static constexpr int kDefaultIntervalMs = 10000;

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings("SyncLens", "SyncLens")
{}

QString SettingsManager::webUiUrl() const
{
    return m_settings.value("webui/url", kDefaultUrl).toString();
}

void SettingsManager::setWebUiUrl(const QString &url)
{
    m_settings.setValue("webui/url", url);
}

bool SettingsManager::startMinimized() const
{
    return m_settings.value("app/startMinimized", false).toBool();
}

void SettingsManager::setStartMinimized(bool value)
{
    m_settings.setValue("app/startMinimized", value);
}

bool SettingsManager::launchOnLogin() const
{
    return m_settings.value("app/launchOnLogin", false).toBool();
}

void SettingsManager::setLaunchOnLogin(bool value)
{
    m_settings.setValue("app/launchOnLogin", value);
}

bool SettingsManager::notificationsEnabled() const
{
    return m_settings.value("notifications/enabled", true).toBool();
}

void SettingsManager::setNotificationsEnabled(bool value)
{
    m_settings.setValue("notifications/enabled", value);
}

int SettingsManager::healthCheckIntervalMs() const
{
    return m_settings.value("health/intervalMs", kDefaultIntervalMs).toInt();
}

void SettingsManager::setHealthCheckIntervalMs(int ms)
{
    m_settings.setValue("health/intervalMs", ms);
}

QSize SettingsManager::windowSize() const
{
    return m_settings.value("window/size", QSize(1024, 768)).toSize();
}

void SettingsManager::setWindowSize(const QSize &size)
{
    m_settings.setValue("window/size", size);
}

QPoint SettingsManager::windowPos() const
{
    return m_settings.value("window/pos", QPoint(100, 100)).toPoint();
}

void SettingsManager::setWindowPos(const QPoint &pos)
{
    m_settings.setValue("window/pos", pos);
}
