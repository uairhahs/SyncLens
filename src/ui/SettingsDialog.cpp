#include "SettingsDialog.h"
#include "settings/SettingsManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(SettingsManager *settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(tr("SyncLens Settings"));
    setMinimumWidth(440);

    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_urlEdit = new QLineEdit(m_settings->webUiUrl(), this);
    form->addRow(tr("WebUI URL:"), m_urlEdit);

    m_startMinimized = new QCheckBox(tr("Start minimized to tray"), this);
    m_startMinimized->setChecked(m_settings->startMinimized());
    form->addRow(QString(), m_startMinimized);

    m_launchOnLogin = new QCheckBox(tr("Launch on login (XDG autostart)"), this);
    m_launchOnLogin->setChecked(m_settings->launchOnLogin());
    form->addRow(QString(), m_launchOnLogin);

    m_notifications = new QCheckBox(tr("Enable desktop notifications"), this);
    m_notifications->setChecked(m_settings->notificationsEnabled());
    form->addRow(QString(), m_notifications);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    root->addLayout(form);
    root->addSpacing(8);
    root->addWidget(buttons);
}

void SettingsDialog::accept()
{
    m_settings->setWebUiUrl(m_urlEdit->text().trimmed());
    m_settings->setStartMinimized(m_startMinimized->isChecked());
    m_settings->setLaunchOnLogin(m_launchOnLogin->isChecked());
    m_settings->setNotificationsEnabled(m_notifications->isChecked());
    QDialog::accept();
}
