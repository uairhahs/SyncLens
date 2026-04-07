#include "AboutDialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QSizePolicy>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About SyncLens"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(480);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(28, 24, 28, 20);

    auto *title = new QLabel(
        "<b style='font-size:16px;'>SyncLens</b>"
        "&nbsp;<span style='color:#888; font-size:13px;'>0.1.0</span>",
        this);
    title->setTextFormat(Qt::RichText);
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(
        tr("An unofficial Qt desktop wrapper for the Resilio Sync WebUI on Linux."),
        this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto *disclaimer = new QLabel(this);
    disclaimer->setText(
        tr("SyncLens is an independent, community-developed project and is "
           "<b>not affiliated with, endorsed by, or sponsored by Resilio, Inc.</b> "
           "\"Resilio\" and \"Resilio Sync\" are trademarks of Resilio, Inc. "
           "All rights reserved by their respective owners."));
    disclaimer->setTextFormat(Qt::RichText);
    disclaimer->setWordWrap(true);
    disclaimer->setAlignment(Qt::AlignCenter);
    disclaimer->setStyleSheet("color: #888; font-size: 11px;");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    layout->addWidget(title);
    layout->addSpacing(2);
    layout->addWidget(subtitle);
    layout->addSpacing(4);
    layout->addWidget(line);
    layout->addSpacing(4);
    layout->addWidget(disclaimer);
    layout->addSpacing(8);
    layout->addWidget(buttons);

    adjustSize();
    setFixedHeight(sizeHint().height());
}
