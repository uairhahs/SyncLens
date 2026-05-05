#pragma once

#include <QDialog>
#include <QList>
#include "settings/SettingsManager.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

// Dialog that lets the user add, remove, and rename Sync instances.
// Call result() after exec() == Accepted to get the updated list.
class InstanceManagerDialog : public QDialog
{
  Q_OBJECT
public:
  explicit InstanceManagerDialog(SettingsManager *settings,
                                 QWidget *parent = nullptr);

  QList<InstanceConfig> result() const { return m_instances; }

private slots:
  void onAdd();
  void onEdit();
  void onRemove();
  void onSelectionChanged();

private:
  void refreshList();
  // Returns the edited config on accept; on cancel, returned id is empty.
  static InstanceConfig promptForConfig(const InstanceConfig &initial,
                                        QWidget *parent);

  SettingsManager *m_settings;
  QList<InstanceConfig> m_instances;
  QListWidget *m_listWidget;
  QPushButton *m_editBtn;
  QPushButton *m_removeBtn;
};
