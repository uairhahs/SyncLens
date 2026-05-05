#include "InstanceManagerDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QUuid>
#include <QUrl>

static bool isValidWebUiUrl(const QString &raw)
{
  const QUrl u(raw);
  return u.isValid() && (u.scheme() == QLatin1String("http") || u.scheme() == QLatin1String("https")) && !u.host().isEmpty();
}

InstanceManagerDialog::InstanceManagerDialog(SettingsManager *settings,
                                             QWidget *parent)
    : QDialog(parent), m_settings(settings), m_instances(settings->instances())
{
  setWindowTitle(tr("Manage Sync Instances"));
  setMinimumWidth(500);
  setMinimumHeight(320);

  auto *root = new QVBoxLayout(this);

  auto *desc = new QLabel(
      tr("Each instance connects to a separate Resilio Sync WebUI "
         "and maintains its own browser session and tray icon."),
      this);
  desc->setWordWrap(true);
  root->addWidget(desc);
  root->addSpacing(8);

  m_listWidget = new QListWidget(this);
  m_listWidget->setAlternatingRowColors(true);
  root->addWidget(m_listWidget);

  // Button row below the list
  auto *btnRow = new QHBoxLayout;
  auto *addBtn = new QPushButton(tr("Add"), this);
  m_editBtn = new QPushButton(tr("Edit"), this);
  m_removeBtn = new QPushButton(tr("Remove"), this);
  btnRow->addWidget(addBtn);
  btnRow->addWidget(m_editBtn);
  btnRow->addWidget(m_removeBtn);
  btnRow->addStretch();
  root->addLayout(btnRow);

  root->addSpacing(8);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);

  connect(addBtn, &QPushButton::clicked,
          this, &InstanceManagerDialog::onAdd);
  connect(m_editBtn, &QPushButton::clicked,
          this, &InstanceManagerDialog::onEdit);
  connect(m_removeBtn, &QPushButton::clicked,
          this, &InstanceManagerDialog::onRemove);
  connect(m_listWidget, &QListWidget::itemSelectionChanged,
          this, &InstanceManagerDialog::onSelectionChanged);
  connect(m_listWidget, &QListWidget::itemDoubleClicked,
          this, &InstanceManagerDialog::onEdit);

  refreshList();
  onSelectionChanged();
}

void InstanceManagerDialog::refreshList()
{
  const int prev = m_listWidget->currentRow();
  m_listWidget->clear();
  for (const InstanceConfig &cfg : std::as_const(m_instances))
    m_listWidget->addItem(cfg.name + QStringLiteral("  \u2014  ") + cfg.url);
  if (prev >= 0 && prev < m_listWidget->count())
    m_listWidget->setCurrentRow(prev);
}

void InstanceManagerDialog::onSelectionChanged()
{
  const bool has = m_listWidget->currentRow() >= 0;
  const bool canDel = has && m_instances.size() > 1;
  m_editBtn->setEnabled(has);
  m_removeBtn->setEnabled(canDel);
}

void InstanceManagerDialog::onAdd()
{
  InstanceConfig blank;
  blank.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  blank.name = tr("New Instance");
  blank.url = QStringLiteral("http://127.0.0.1:8888/gui/");

  const InstanceConfig cfg = promptForConfig(blank, this);
  if (cfg.id.isEmpty())
    return; // cancelled

  m_instances.append(cfg);
  refreshList();
  m_listWidget->setCurrentRow(static_cast<int>(m_instances.size()) - 1);
  onSelectionChanged();
}

void InstanceManagerDialog::onEdit()
{
  const int row = m_listWidget->currentRow();
  if (row < 0 || row >= static_cast<int>(m_instances.size()))
    return;

  const InstanceConfig cfg = promptForConfig(m_instances.at(row), this);
  if (cfg.id.isEmpty())
    return; // cancelled

  m_instances[row] = cfg;
  refreshList();
  m_listWidget->setCurrentRow(row);
}

void InstanceManagerDialog::onRemove()
{
  const int row = m_listWidget->currentRow();
  if (row < 0 || row >= static_cast<int>(m_instances.size()))
    return;
  if (m_instances.size() <= 1)
  {
    QMessageBox::information(this, tr("Cannot Remove"),
                             tr("At least one instance must remain."));
    return;
  }
  m_instances.removeAt(row);
  refreshList();
  m_listWidget->setCurrentRow(qBound(0, row, static_cast<int>(m_instances.size()) - 1));
  onSelectionChanged();
}

InstanceConfig InstanceManagerDialog::promptForConfig(const InstanceConfig &initial,
                                                      QWidget *parent)
{
  QDialog dlg(parent);
  dlg.setWindowTitle(tr("Instance Settings"));
  dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
  dlg.setMinimumWidth(400);

  auto *form = new QFormLayout(&dlg);
  auto *nameEdit = new QLineEdit(initial.name, &dlg);
  auto *urlEdit = new QLineEdit(initial.url, &dlg);
  form->addRow(tr("Name:"), nameEdit);
  form->addRow(tr("WebUI URL:"), urlEdit);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  // Validate URL before accepting — do not wire accepted() directly.
  auto *okBtn = buttons->button(QDialogButtonBox::Ok);
  QObject::connect(okBtn, &QPushButton::clicked, &dlg, [&]()
                   {
      const QString url = urlEdit->text().trimmed();
      if (!isValidWebUiUrl(url)) {
          QMessageBox::warning(&dlg, tr("Invalid URL"),
                               tr("Please enter a valid http:// or https:// URL.\n"
                                  "Example: http://127.0.0.1:8888/gui/"));
          return;
      }
      dlg.accept(); });
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  form->addRow(buttons);

  nameEdit->selectAll();
  nameEdit->setFocus();

  if (dlg.exec() != QDialog::Accepted)
    return {}; // empty id signals cancellation

  InstanceConfig result = initial; // preserves the id
  result.name = nameEdit->text().trimmed();
  result.url = urlEdit->text().trimmed();
  return result;
}
