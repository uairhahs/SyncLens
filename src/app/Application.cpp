#include "Application.h"
#include "SyncInstance.h"
#include "settings/SettingsManager.h"
#include "ui/InstanceManagerDialog.h"

#include <QIcon>
#include <QMap>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName("SyncLens");
    setOrganizationName("SyncLens");
    setQuitOnLastWindowClosed(false);
    // Use the theme's icon when present so icon themes can restyle it.
    setWindowIcon(QIcon::fromTheme("synclens", QIcon(":/icons/synclens.svg")));

    m_settings = new SettingsManager(this);
}

Application::~Application()
{
    qDeleteAll(m_instances);
}

int Application::run()
{
    createInstances();
    const bool minimized = m_settings->startMinimized();
    for (SyncInstance *inst : std::as_const(m_instances))
        inst->start(minimized);
    return exec();
}

void Application::createInstances()
{
    const QList<InstanceConfig> configs = m_settings->instances();
    for (const InstanceConfig &cfg : configs)
    {
        auto *inst = new SyncInstance(cfg, m_settings);
        connect(inst, &SyncInstance::manageInstancesRequested,
                this, &Application::onManageInstancesRequested);
        m_instances.append(inst);
    }
}

void Application::onManageInstancesRequested()
{
    // Guard: only one dialog at a time. All MainWindows share the same slot,
    // so two windows could otherwise open conflicting dialogs simultaneously.
    if (m_manageDialogOpen)
        return;
    m_manageDialogOpen = true;
    InstanceManagerDialog dlg(m_settings);
    if (dlg.exec() == QDialog::Accepted)
        applyInstanceChanges(dlg.result());
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
            auto *inst = new SyncInstance(cfg, m_settings);
            connect(inst, &SyncInstance::manageInstancesRequested,
                    this, &Application::onManageInstancesRequested);
            inst->start(false);
            updated.append(inst);
        }
    }

    // Destroy instances that were removed.
    qDeleteAll(existing);

    m_instances = updated;
}
