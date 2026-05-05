#include "TrayController.h"

#include <QMenu>
#include <QAction>
#include <QIcon>

TrayController::TrayController(QObject *parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(this);
    m_menu = new QMenu;

    m_openAction = m_menu->addAction(tr("Open SyncLens"));
    m_openBrowserAction = m_menu->addAction(tr("Open in Browser"));
    m_refreshAction = m_menu->addAction(tr("Refresh Status"));
    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(tr("Quit"));

    m_tray->setContextMenu(m_menu);

    connect(m_openAction, &QAction::triggered,
            this, &TrayController::openWindowRequested);
    connect(m_openBrowserAction, &QAction::triggered,
            this, &TrayController::openInBrowserRequested);
    connect(m_refreshAction, &QAction::triggered,
            this, &TrayController::refreshRequested);
    connect(m_quitAction, &QAction::triggered,
            this, &TrayController::quitRequested);
    connect(m_tray, &QSystemTrayIcon::activated,
            this, &TrayController::onActivated);

    updateState(HealthState::Unknown);
}

void TrayController::show()
{
    m_tray->show();
}

void TrayController::updateState(HealthState state)
{
    QString iconPath;
    switch (state)
    {
    case HealthState::Running:
        iconPath = ":/icons/synclens-healthy.png";
        break;
    case HealthState::Starting:
    case HealthState::Unknown:
        iconPath = ":/icons/synclens-connecting.png";
        break;
    case HealthState::Stopped:
        iconPath = ":/icons/synclens-stopped.png";
        break;
    case HealthState::Unreachable:
    case HealthState::Error:
        iconPath = ":/icons/synclens-error.png";
        break;
    }

    m_tray->setIcon(QIcon(iconPath));
    rebuildTooltip(state);
}

void TrayController::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger ||
        reason == QSystemTrayIcon::DoubleClick)
    {
        emit openWindowRequested();
    }
}

void TrayController::setLabel(const QString &label)
{
    m_label = label;
}

void TrayController::rebuildTooltip(HealthState state)
{
    QString prefix = QStringLiteral("SyncLens");
    if (!m_label.isEmpty())
        prefix += QStringLiteral(" — ") + m_label;
    QString tip = prefix + QStringLiteral(": ");
    switch (state)
    {
    case HealthState::Running:
        tip += tr("Running");
        break;
    case HealthState::Starting:
        tip += tr("Starting...");
        break;
    case HealthState::Stopped:
        tip += tr("Stopped");
        break;
    case HealthState::Unreachable:
        tip += tr("Unreachable");
        break;
    case HealthState::Error:
        tip += tr("Error");
        break;
    case HealthState::Unknown:
        tip += tr("Unknown");
        break;
    }
    m_tray->setToolTip(tip);
}
