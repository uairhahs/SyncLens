#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

static constexpr const char *kDefaultUrl = "http://127.0.0.1:8888/gui/";
static constexpr int kDefaultIntervalMs = 10000;

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), m_settings("SyncLens", "SyncLens")
{
}

// ---- Instance list --------------------------------------------------------

QList<InstanceConfig> SettingsManager::instances() const
{
    const QStringList order = m_settings.value("instances/order").toStringList();
    if (order.isEmpty())
    {
        // Migrate from legacy single-URL config.
        InstanceConfig def;
        def.id = QStringLiteral("default");
        def.name = QStringLiteral("Default");
        def.url = m_settings.value("webui/url", kDefaultUrl).toString();
        return {def};
    }
    QList<InstanceConfig> result;
    for (const QString &id : std::as_const(order))
    {
        InstanceConfig cfg;
        cfg.id = id;
        cfg.name = m_settings.value("instances/" + id + "/name", id).toString();
        cfg.url = m_settings.value("instances/" + id + "/url", kDefaultUrl).toString();
        result.append(cfg);
    }
    return result;
}

void SettingsManager::setInstances(const QList<InstanceConfig> &list)
{
    // Remove data for instances no longer present.
    const QStringList oldOrder = m_settings.value("instances/order").toStringList();
    for (const QString &id : std::as_const(oldOrder))
        m_settings.remove("instances/" + id);

    QStringList order;
    for (const InstanceConfig &cfg : list)
    {
        order.append(cfg.id);
        m_settings.setValue("instances/" + cfg.id + "/name", cfg.name);
        m_settings.setValue("instances/" + cfg.id + "/url", cfg.url);
    }
    m_settings.setValue("instances/order", order);
}

// ---- Global settings ------------------------------------------------------

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

    // Write or remove the XDG autostart entry so the desktop environment
    // actually starts SyncLens on login, regardless of the session type.
    const QString autostartDir =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart");
    const QString desktopPath = autostartDir + QStringLiteral("/synclens.desktop");

    if (value)
    {
        QDir().mkpath(autostartDir);
        QFile f(desktopPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&f);
            out << "[Desktop Entry]\n"
                << "Name=SyncLens\n"
                << "Comment=Unofficial Qt wrapper for the Resilio Sync WebUI\n"
                << "Exec=synclens\n"
                << "Icon=synclens\n"
                << "Type=Application\n"
                << "Categories=Network;FileTransfer;\n"
                << "StartupNotify=false\n"
                << "X-GNOME-Autostart-enabled=true\n";
        }
    }
    else
    {
        QFile::remove(desktopPath);
    }
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

QString SettingsManager::theme() const
{
    return m_settings.value("webui/theme").toString();
}

void SettingsManager::setTheme(const QString &id)
{
    if (id == theme())
        return;
    m_settings.setValue("webui/theme", id);
    emit themeChanged();
}

bool SettingsManager::checkForUpdates() const
{
    return m_settings.value("updates/check", true).toBool();
}

void SettingsManager::setCheckForUpdates(bool value)
{
    m_settings.setValue("updates/check", value);
}

QDateTime SettingsManager::lastUpdateCheck() const
{
    return m_settings.value("updates/lastCheck").toDateTime();
}

void SettingsManager::setLastUpdateCheck(const QDateTime &when)
{
    m_settings.setValue("updates/lastCheck", when);
}

QString SettingsManager::lastNotifiedVersion() const
{
    return m_settings.value("updates/lastNotified").toString();
}

void SettingsManager::setLastNotifiedVersion(const QString &version)
{
    m_settings.setValue("updates/lastNotified", version);
}

// ---- Per-instance window geometry -----------------------------------------

QSize SettingsManager::windowSize(const QString &instanceId) const
{
    return m_settings.value("window/" + instanceId + "/size", QSize(1024, 768)).toSize();
}

void SettingsManager::setWindowSize(const QString &instanceId, const QSize &size)
{
    m_settings.setValue("window/" + instanceId + "/size", size);
}

QPoint SettingsManager::windowPos(const QString &instanceId) const
{
    return m_settings.value("window/" + instanceId + "/pos", QPoint(100, 100)).toPoint();
}

void SettingsManager::setWindowPos(const QString &instanceId, const QPoint &pos)
{
    m_settings.setValue("window/" + instanceId + "/pos", pos);
}
