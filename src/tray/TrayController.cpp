#include "TrayController.h"

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace
{
// Fallback for when the themed icon isn't installed: renders the bundled copy
// with the arrows in the palette's text colour. Keep in sync with TEXT in
// resources/icons/generate_icons.py.
QIcon bundledTrayIcon(const QString &name)
{
    QFile file(":/icons/" + name + ".svg");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray text = QApplication::palette().color(QPalette::WindowText).name().toLatin1();
    QSvgRenderer renderer(file.readAll().replace("#DFDFDF", text));

    QIcon icon;
    for (int size : {16, 22, 24, 32, 48, 64, 128, 256})
    {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}
} // namespace

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

    qApp->installEventFilter(this);
    updateState(HealthState::Unknown);
}

bool TrayController::eventFilter(QObject *watched, QEvent *event)
{
    // Re-tint when the colour scheme changes (e.g. switching light/dark).
    if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange)
        applyIcon();
    return QObject::eventFilter(watched, event);
}

void TrayController::show()
{
    m_tray->show();
}

void TrayController::updateState(HealthState state)
{
    m_state = state;
    applyIcon();
    rebuildTooltip(state);
}

void TrayController::applyIcon()
{
    QString name;
    switch (m_state)
    {
    case HealthState::Running:
        name = "healthy";
        break;
    case HealthState::Starting:
    case HealthState::Unknown:
        name = "connecting";
        break;
    case HealthState::Stopped:
        name = "stopped";
        break;
    case HealthState::Unreachable:
    case HealthState::Error:
        name = "error";
        break;
    }

    // Prefer the installed themed icon: the tray host loads it by name and
    // paints it in the panel's own colours.
    const QString themeName = "synclens-tray-" + name;
    if (QIcon::hasThemeIcon(themeName))
        m_tray->setIcon(QIcon::fromTheme(themeName));
    else
        m_tray->setIcon(bundledTrayIcon(themeName));
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
