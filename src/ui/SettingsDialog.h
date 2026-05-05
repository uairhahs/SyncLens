#pragma once

#include <QDialog>
#include "settings/SettingsManager.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(SettingsManager *settings, InstanceConfig *config,
                            QWidget *parent = nullptr);

private slots:
    void accept() override;

private:
    SettingsManager *m_settings;
    InstanceConfig *m_config;
    QLineEdit *m_nameEdit;
    QLineEdit *m_urlEdit;
    QCheckBox *m_startMinimized;
    QCheckBox *m_launchOnLogin;
    QCheckBox *m_notifications;
};
