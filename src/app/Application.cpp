#include "Application.h"
#include "SyncInstance.h"
#include "UpdateChecker.h"
#include "theme/ThemeManager.h"
#include "settings/SettingsManager.h"
#include "settings/CredentialStore.h"
#include "tray/TrayController.h"
#include "ui/InstanceManagerDialog.h"

#include <algorithm>

#include <QDateTime>
#include <QDesktopServices>
#include <QIcon>
#include <QMap>
#include <QMessageBox>
#include <QUrl>

namespace
{
constexpr int kUpdateIntervalMs = 24 * 60 * 60 * 1000;
// Delay the first automatic check so it doesn't compete with startup.
constexpr int kFirstUpdateCheckDelayMs = 60 * 1000;
} // namespace

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName("SyncLens");
    setOrganizationName("SyncLens");
    setApplicationVersion(UpdateChecker::currentVersion());
    setQuitOnLastWindowClosed(false);
    // Use the theme's icon when present so icon themes can restyle it.
    setWindowIcon(QIcon::fromTheme("synclens", QIcon(":/icons/synclens.svg")));

    m_settings = new SettingsManager(this);

    // One tray icon for the whole app, whatever the number of instances.
    m_tray = new TrayController(this);
    auto forInstance = [this](void (SyncInstance::*fn)())
    {
        return [this, fn](const QString &id)
        {
            if (SyncInstance *inst = instanceById(id))
                (inst->*fn)();
        };
    };
    connect(m_tray, &TrayController::openWindowRequested,
            this, &Application::switchTo);
    connect(m_tray, &TrayController::openInBrowserRequested,
            this, forInstance(&SyncInstance::openInBrowser));
    connect(m_tray, &TrayController::refreshRequested,
            this, forInstance(&SyncInstance::refreshStatus));
    connect(m_tray, &TrayController::reconnectRequested,
            this, forInstance(&SyncInstance::reconnect));
    connect(m_tray, &TrayController::notificationClicked, this, [](const QString &url)
            { QDesktopServices::openUrl(QUrl(url)); });
    connect(m_tray, &TrayController::quitRequested,
            this, &QApplication::quit);

    m_themes = new ThemeManager(this);
    connect(m_settings, &SettingsManager::themeChanged, this, [this]()
            { applyTheme(); });
    connect(m_themes, &ThemeManager::filesChanged, this, [this]()
            { applyTheme(); });

    m_updateChecker = new UpdateChecker(this);
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &Application::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::upToDate,
            this, &Application::onUpToDate);
    connect(m_updateChecker, &UpdateChecker::failed,
            this, &Application::onUpdateCheckFailed);
    m_updateTimer.setSingleShot(true);
    connect(&m_updateTimer, &QTimer::timeout, this, [this]()
            {
        if (m_settings->checkForUpdates())
        {
            m_settings->setLastUpdateCheck(QDateTime::currentDateTimeUtc());
            m_updateChecker->check();
        }
        scheduleUpdateCheck(); });
}

Application::~Application()
{
    qDeleteAll(m_instances);
}

int Application::run()
{
    createInstances();
    syncTrayInstances();
    m_tray->show();
    applyTheme(true);
    for (SyncInstance *inst : std::as_const(m_instances))
        inst->start();
    if (!m_settings->startMinimized())
        switchTo(m_settings->defaultInstanceId());
    scheduleUpdateCheck();
    return exec();
}

SyncInstance *Application::addInstance(const InstanceConfig &cfg)
{
    auto *inst = new SyncInstance(cfg, m_settings);
    connect(inst, &SyncInstance::manageInstancesRequested,
            this, &Application::onManageInstancesRequested);
    connect(inst, &SyncInstance::checkForUpdatesRequested,
            this, &Application::onCheckForUpdatesRequested);
    connect(inst, &SyncInstance::healthStateChanged,
            m_tray, &TrayController::updateState);
    connect(inst, &SyncInstance::configChanged,
            this, &Application::syncTrayInstances);
    return inst;
}

SyncInstance *Application::instanceById(const QString &id) const
{
    for (SyncInstance *inst : m_instances)
    {
        if (inst->config().id == id)
            return inst;
    }
    return nullptr;
}

void Application::switchTo(const QString &id)
{
    SyncInstance *target = instanceById(id);
    if (!target)
        return;
    for (SyncInstance *inst : std::as_const(m_instances))
    {
        if (inst != target)
            inst->hideWindow();
    }
    target->showWindow();
}

void Application::syncTrayInstances()
{
    QList<TrayController::Entry> entries;
    for (SyncInstance *inst : std::as_const(m_instances))
        entries.append({inst->config().id, inst->config().name, inst->healthState()});
    m_tray->setInstances(entries);
    m_tray->setDefaultInstance(m_settings->defaultInstanceId());
}

void Application::createInstances()
{
    const QList<InstanceConfig> configs = m_settings->instances();
    for (const InstanceConfig &cfg : configs)
        m_instances.append(addInstance(cfg));
}

void Application::onManageInstancesRequested()
{
    // Guard: only one dialog at a time. All MainWindows share the same slot,
    // so two windows could otherwise open conflicting dialogs simultaneously.
    if (m_manageDialogOpen)
        return;
    m_manageDialogOpen = true;
    // Preselect the instance whose window asked, so OK alone stays put.
    const auto *caller = qobject_cast<SyncInstance *>(sender());
    InstanceManagerDialog dlg(m_settings, caller ? caller->config().id : QString());
    if (dlg.exec() == QDialog::Accepted)
    {
        m_settings->setDefaultInstanceId(dlg.defaultId());
        applyInstanceChanges(dlg.result());
        // Switch to the highlighted instance. Done after applying, as a
        // newly added instance only exists from here on.
        if (instanceById(dlg.selectedId()))
            switchTo(dlg.selectedId());
    }
    m_manageDialogOpen = false;
}

void Application::applyInstanceChanges(const QList<InstanceConfig> &newList)
{
    m_settings->setInstances(newList);

    // Build a lookup of current instances by id.
    QMap<QString, SyncInstance *> existing;
    for (SyncInstance *inst : std::as_const(m_instances))
        existing.insert(inst->config().id, inst);

    QList<SyncInstance *> updated;
    for (const InstanceConfig &cfg : newList)
    {
        if (existing.contains(cfg.id))
        {
            SyncInstance *inst = existing.take(cfg.id);
            inst->applyConfig(cfg);
            updated.append(inst);
        }
        else
        {
            SyncInstance *inst = addInstance(cfg);
            if (!m_themeScript.isEmpty())
                inst->setThemeScript(m_themeScript);
            inst->start(); // shown only if switched to
            updated.append(inst);
        }
    }

    // Destroy instances that were removed, and forget their saved login
    // unless another instance still uses the same URL.
    for (SyncInstance *inst : std::as_const(existing))
    {
        const QString url = inst->config().url;
        const bool stillUsed = std::any_of(newList.cbegin(), newList.cend(),
                                           [&url](const InstanceConfig &c)
                                           { return c.url == url; });
        if (!stillUsed)
            CredentialStore::clear(url);
    }
    qDeleteAll(existing);

    m_instances = updated;
    syncTrayInstances();
}

// ---- WebUI theme ----------------------------------------------------------

void Application::applyTheme(bool force)
{
    // File watch events also fire for unrelated writes in the config dir
    // (settings, window geometry), so only push a script that changed.
    const QString script = ThemeManager::script(m_settings->theme());
    if (!force && script == m_themeScript)
        return;
    m_themeScript = script;
    for (SyncInstance *inst : std::as_const(m_instances))
        inst->setThemeScript(script);
}

// ---- Update checks --------------------------------------------------------

void Application::scheduleUpdateCheck()
{
    // Check once a day, counting from the last check so restarting SyncLens
    // doesn't trigger one every time.
    qint64 wait = kFirstUpdateCheckDelayMs;
    const QDateTime last = m_settings->lastUpdateCheck();
    if (last.isValid())
    {
        const qint64 due = last.msecsTo(QDateTime::currentDateTimeUtc());
        wait = qMax<qint64>(kFirstUpdateCheckDelayMs, kUpdateIntervalMs - due);
    }
    m_updateTimer.start(int(qMin<qint64>(wait, kUpdateIntervalMs)));
}

void Application::onCheckForUpdatesRequested(QWidget *parent)
{
    m_interactiveCheck = true;
    m_interactiveParent = parent;
    m_settings->setLastUpdateCheck(QDateTime::currentDateTimeUtc());
    m_updateChecker->check();
}

void Application::onUpdateAvailable(const QString &version, const QString &url)
{
    if (m_interactiveCheck)
    {
        m_interactiveCheck = false;
        const auto answer = QMessageBox::information(
            m_interactiveParent, tr("Update available"),
            tr("SyncLens %1 is available (you have %2).\n\n"
               "Open the release page?")
                .arg(version, UpdateChecker::currentVersion()),
            QMessageBox::Open | QMessageBox::Close, QMessageBox::Open);
        if (answer == QMessageBox::Open)
            QDesktopServices::openUrl(QUrl(url));
        return;
    }

    // Automatic check: notify once per release, from the tray icon.
    if (!m_settings->notificationsEnabled() || m_instances.isEmpty() ||
        m_settings->lastNotifiedVersion() == version)
        return;
    m_settings->setLastNotifiedVersion(version);
    m_tray->notify(
        tr("SyncLens %1 is available").arg(version),
        tr("You have %1. Click to open the release page.")
            .arg(UpdateChecker::currentVersion()),
        url);
}

void Application::onUpToDate(const QString &latest)
{
    if (!m_interactiveCheck)
        return;
    m_interactiveCheck = false;
    QMessageBox::information(m_interactiveParent, tr("No updates"),
                             tr("SyncLens %1 is the latest version.")
                                 .arg(UpdateChecker::currentVersion()));
    Q_UNUSED(latest);
}

void Application::onUpdateCheckFailed(const QString &error)
{
    // Automatic checks fail silently; they will retry tomorrow.
    if (!m_interactiveCheck)
        return;
    m_interactiveCheck = false;
    QMessageBox::warning(m_interactiveParent, tr("Update check failed"),
                         tr("Could not check for updates:\n%1").arg(error));
}
