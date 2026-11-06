#include "SettingsDialog.h"
#include "settings/SettingsManager.h"
#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QUrl>

static bool isValidWebUiUrl(const QString &raw)
{
    const QUrl u(raw);
    return u.isValid() && (u.scheme() == QLatin1String("http") || u.scheme() == QLatin1String("https")) && !u.host().isEmpty();
}

SettingsDialog::SettingsDialog(SettingsManager *settings, InstanceConfig *config,
                               QWidget *parent)
    : QDialog(parent), m_settings(settings), m_config(config)
{
    setWindowTitle(tr("SyncLens Settings"));
    setMinimumWidth(440);

    auto *root = new QVBoxLayout(this);

    // ---- Per-instance settings ----
    auto *instanceForm = new QFormLayout;
    m_nameEdit = new QLineEdit(m_config->name, this);
    instanceForm->addRow(tr("Name:"), m_nameEdit);
    m_urlEdit = new QLineEdit(m_config->url, this);
    instanceForm->addRow(tr("WebUI URL:"), m_urlEdit);
    root->addLayout(instanceForm);

    // Divider between per-instance and global sections
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    root->addWidget(line);

    // ---- Global settings ----
    auto *globalForm = new QFormLayout;
    m_startMinimized = new QCheckBox(tr("Start minimized to tray"), this);
    m_startMinimized->setChecked(m_settings->startMinimized());
    globalForm->addRow(QString(), m_startMinimized);

    m_launchOnLogin = new QCheckBox(tr("Launch on login (XDG autostart)"), this);
    m_launchOnLogin->setChecked(m_settings->launchOnLogin());
    globalForm->addRow(QString(), m_launchOnLogin);

    m_notifications = new QCheckBox(tr("Enable desktop notifications"), this);
    m_notifications->setChecked(m_settings->notificationsEnabled());
    globalForm->addRow(QString(), m_notifications);

    m_checkForUpdates = new QCheckBox(tr("Check for updates daily"), this);
    m_checkForUpdates->setChecked(m_settings->checkForUpdates());
    globalForm->addRow(QString(), m_checkForUpdates);

    m_theme = new QComboBox(this);
    m_theme->addItem(tr("Resilio default"), QString());
    const QList<ThemeInfo> themes = ThemeManager::available();
    for (const ThemeInfo &t : themes)
        m_theme->addItem(t.user ? tr("%1 (custom)").arg(t.name) : t.name, t.id);
    const int current = m_theme->findData(m_settings->theme());
    m_theme->setCurrentIndex(current < 0 ? 0 : current);
    m_theme->setToolTip(tr("Recolours the WebUI. A theme also picks light or dark, "
                           "overriding Resilio's Appearance setting."));

    auto *themesFolder = new QPushButton(tr("Open Themes Folder"), this);
    themesFolder->setToolTip(tr("Add your own themes here as .json files. "
                                "user.css in the folder above is applied on top."));
    connect(themesFolder, &QPushButton::clicked, this, []()
            {
        QDir().mkpath(ThemeManager::userThemesDir());
        QDesktopServices::openUrl(QUrl::fromLocalFile(ThemeManager::userThemesDir())); });

    auto *themeRow = new QHBoxLayout;
    themeRow->addWidget(m_theme, 1);
    themeRow->addWidget(themesFolder);
    globalForm->addRow(tr("WebUI theme:"), themeRow);

    root->addLayout(globalForm);

    root->addSpacing(8);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void SettingsDialog::accept()
{
    const QString url = m_urlEdit->text().trimmed();
    if (!isValidWebUiUrl(url))
    {
        QMessageBox::warning(this, tr("Invalid URL"),
                             tr("Please enter a valid http:// or https:// URL.\n"
                                "Example: http://127.0.0.1:8888/gui/"));
        return; // keep dialog open
    }
    m_config->name = m_nameEdit->text().trimmed();
    m_config->url = url;
    m_settings->setStartMinimized(m_startMinimized->isChecked());
    m_settings->setLaunchOnLogin(m_launchOnLogin->isChecked());
    m_settings->setNotificationsEnabled(m_notifications->isChecked());
    m_settings->setCheckForUpdates(m_checkForUpdates->isChecked());
    m_settings->setTheme(m_theme->currentData().toString());
    QDialog::accept();
}
