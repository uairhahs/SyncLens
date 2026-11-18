#pragma once

#include <QObject>
#include <QString>

#include <memory>

class QLocalServer;
class QLockFile;

// Keeps one SyncLens per user. The first copy holds a lock file and listens on
// a local socket in the runtime directory; a later copy asks it to show its
// window and exits, so launching from the menu, autostart and the systemd
// service can't stack up tray icons.
class SingleInstance : public QObject
{
    Q_OBJECT
public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance() override;

    // True if this process is the first copy and now holds the lock. False if
    // another copy is running; it has then been asked to show its window.
    bool claim();

signals:
    // A later launch asked for the window. Carries that launch's Wayland
    // activation token, if it had one, so the window may take focus.
    void activationRequested(const QString &activationToken);

private:
    void listen();
    void notifyRunningCopy() const;

    QString m_socketPath;
    std::unique_ptr<QLockFile> m_lock;
    QLocalServer *m_server = nullptr;
};
