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
#include <QRectF>
#include <QColor>
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

QString stateText(HealthState state)
{
    switch (state)
    {
    case HealthState::Running:
        return TrayController::tr("Running");
    case HealthState::Starting:
        return TrayController::tr("Starting...");
    case HealthState::Stopped:
        return TrayController::tr("Stopped");
    case HealthState::Unreachable:
        return TrayController::tr("Unreachable");
    case HealthState::Error:
        return TrayController::tr("Error");
    case HealthState::Unknown:
        break;
    }
    return TrayController::tr("Unknown");
}

// Same colours as the tray icon's centre marks (resources/icons/generate_icons.py).
QColor stateColor(HealthState state)
{
    switch (state)
    {
    case HealthState::Running:
        return QColor(0x02, 0xFD, 0xC2);
    case HealthState::Starting:
    case HealthState::Unknown:
        return QColor(0xFB, 0xBF, 0x24);
    case HealthState::Stopped:
        return QColor(0x60, 0xA5, 0xFA);
    case HealthState::Unreachable:
    case HealthState::Error:
        return QColor(0xF8, 0x71, 0x71);
    }
    return QColor(0xFB, 0xBF, 0x24);
}

// Filled circle for a menu entry, rendered at a few sizes for HiDPI.
QIcon statusDot(HealthState state)
{
    QIcon icon;
    for (int size : {16, 22, 32, 48})
    {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(stateColor(state));
        const qreal d = size * 0.5;
        painter.drawEllipse(QRectF((size - d) / 2, (size - d) / 2, d, d));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

// Higher = worse; the tray icon reflects the worst instance.
int severity(HealthState state)
{
    switch (state)
    {
    case HealthState::Running:
        return 0;
    case HealthState::Starting:
    case HealthState::Unknown:
        return 1;
    case HealthState::Stopped:
        return 2;
    case HealthState::Unreachable:
    case HealthState::Error:
        return 3;
    }
    return 1;
}
} // namespace

TrayController::TrayController(QObject *parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(this);
    m_menu = new QMenu;
    m_tray->setContextMenu(m_menu);

    connect(m_tray, &QSystemTrayIcon::messageClicked, this, [this]()
            {
        if (!m_notifyUrl.isEmpty())
            emit notificationClicked(m_notifyUrl); });
    connect(m_tray, &QSystemTrayIcon::activated,
            this, &TrayController::onActivated);

    qApp->installEventFilter(this);
    rebuildMenu();
}

TrayController::~TrayController()
{
    delete m_menu; // not a QObject child: QMenu is a top-level widget
}

bool TrayController::eventFilter(QObject *watched, QEvent *event)
{
    // Re-tint when the colour scheme changes (e.g. switching light/dark).
    if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange)
        refresh();
    return QObject::eventFilter(watched, event);
}

void TrayController::show()
{
    m_tray->show();
}

void TrayController::setInstances(const QList<Entry> &entries)
{
    m_entries = entries;
    rebuildMenu();
}

void TrayController::setDefaultInstance(const QString &id)
{
    m_defaultId = id;
    refresh();
}

void TrayController::updateState(const QString &id, HealthState state)
{
    for (Entry &e : m_entries)
    {
        if (e.id == id)
        {
            e.state = state;
            break;
        }
    }
    refresh();
}

void TrayController::addInstanceActions(QMenu *menu, const QString &id)
{
    connect(menu->addAction(tr("Open SyncLens")), &QAction::triggered,
            this, [this, id]()
            { emit openWindowRequested(id); });
    connect(menu->addAction(tr("Refresh Status")), &QAction::triggered,
            this, [this, id]()
            { emit refreshRequested(id); });
    connect(menu->addAction(tr("Reconnect")), &QAction::triggered,
            this, [this, id]()
            { emit reconnectRequested(id); });
}

void TrayController::rebuildMenu()
{
    m_instanceActions.clear(); // owned by m_menu, freed by clear()
    m_menu->clear();

    if (m_entries.size() == 1)
    {
        addInstanceActions(m_menu, m_entries.first().id);
    }
    else
    {
        // One entry per instance; choosing it opens that window.
        for (const Entry &e : std::as_const(m_entries))
        {
            QAction *action = m_menu->addAction(e.name);
            const QString id = e.id;
            connect(action, &QAction::triggered, this, [this, id]()
                    { emit openWindowRequested(id); });
            m_instanceActions.append(action);
        }
    }

    m_menu->addSeparator();
    connect(m_menu->addAction(tr("Quit")), &QAction::triggered,
            this, &TrayController::quitRequested);

    refresh();
}

HealthState TrayController::aggregateState() const
{
    if (m_entries.isEmpty())
        return HealthState::Unknown;
    HealthState worst = m_entries.first().state;
    for (const Entry &e : m_entries)
    {
        if (severity(e.state) > severity(worst))
            worst = e.state;
    }
    return worst;
}

void TrayController::refresh()
{
    const HealthState state = aggregateState();

    QString name;
    switch (state)
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

    // Tooltip: one line per instance, or the familiar single line.
    QString tip;
    if (m_entries.size() == 1)
    {
        const Entry &e = m_entries.first();
        tip = QStringLiteral("SyncLens");
        if (!e.name.isEmpty())
            tip += QStringLiteral(" - ") + e.name;
        tip += QStringLiteral(": ") + stateText(e.state);
    }
    else
    {
        tip = QStringLiteral("SyncLens");
        for (const Entry &e : std::as_const(m_entries))
            tip += QLatin1Char('\n') + e.name + QStringLiteral(": ") + stateText(e.state);
    }
    m_tray->setToolTip(tip);

    for (qsizetype i = 0; i < m_instanceActions.size() && i < m_entries.size(); ++i)
    {
        const Entry &e = m_entries.at(i);
        QString title = e.name + QStringLiteral(" - ") + stateText(e.state);
        if (e.id == m_defaultId)
            title += QStringLiteral(" ") + tr("(default)");
        m_instanceActions.at(i)->setText(title);
        m_instanceActions.at(i)->setIcon(statusDot(e.state));
        // Some desktops hide menu icons by default; the dot is the point here.
        m_instanceActions.at(i)->setIconVisibleInMenu(true);
    }
}

void TrayController::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Trigger &&
        reason != QSystemTrayIcon::DoubleClick)
        return;
    // Left click opens the default instance; the right-click menu reaches
    // the others.
    QString id = m_defaultId;
    if (id.isEmpty() && !m_entries.isEmpty())
        id = m_entries.first().id;
    if (!id.isEmpty())
        emit openWindowRequested(id);
}

void TrayController::notify(const QString &title, const QString &message,
                            const QString &url)
{
    m_notifyUrl = url;
    m_tray->showMessage(title, message, QIcon::fromTheme("synclens", QIcon(":/icons/synclens.svg")));
}
