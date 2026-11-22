#pragma once

#include <QObject>

class QSocketNotifier;

// Turns SIGTERM and SIGINT into a normal QCoreApplication::quit(), so a
// stop from systemd, the installer or Ctrl+C still saves settings and lets
// the web profiles write their cookies to disk.
class QuitOnSignal : public QObject
{
    Q_OBJECT
public:
    explicit QuitOnSignal(QObject *parent = nullptr);
    ~QuitOnSignal() override;

private:
    QSocketNotifier *m_notifier = nullptr;
};
