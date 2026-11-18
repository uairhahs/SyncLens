#include "SingleInstance.h"

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QThread>
#include <QtGlobal>

namespace
{
constexpr QByteArrayView kActivate = "activate";
// The first copy may still be starting and not listening yet.
constexpr int kConnectAttempts = 10;
constexpr int kConnectTimeoutMs = 300;
constexpr int kRetryDelayMs = 200;
} // namespace

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
    // XDG_RUNTIME_DIR: private to the user and cleared at logout.
    const QDir dir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));
    m_socketPath = dir.filePath("synclens.socket");
    m_lock = std::make_unique<QLockFile>(dir.filePath("synclens.lock"));
    // A lock is stale only when its owner has died, however long it ran.
    m_lock->setStaleLockTime(0);
}

SingleInstance::~SingleInstance() = default;

bool SingleInstance::claim()
{
    // tryLock() takes over a lock left by a crashed copy on its own.
    if (!m_lock->tryLock(0))
    {
        notifyRunningCopy();
        return false;
    }
    listen();
    return true;
}

void SingleInstance::listen()
{
    // Holding the lock means no live copy owns the socket, so any file there
    // is left over from a crash.
    QLocalServer::removeServer(m_socketPath);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_socketPath))
    {
        qWarning("SyncLens: cannot listen on %s: %s; a second launch will exit without showing this window",
                 qPrintable(m_socketPath), qPrintable(m_server->errorString()));
        return;
    }

    connect(m_server, &QLocalServer::newConnection, this, [this]()
            {
        while (QLocalSocket *client = m_server->nextPendingConnection())
        {
            connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            connect(client, &QLocalSocket::readyRead, this, [this, client]()
                    {
                while (client->canReadLine())
                {
                    const QByteArray line = client->readLine().trimmed();
                    if (!line.startsWith(kActivate))
                        continue;
                    emit activationRequested(QString::fromUtf8(line.mid(kActivate.size()).trimmed()));
                } });
        } });
}

void SingleInstance::notifyRunningCopy() const
{
    QLocalSocket socket;
    for (int attempt = 0; attempt < kConnectAttempts; ++attempt)
    {
        socket.connectToServer(m_socketPath);
        if (socket.waitForConnected(kConnectTimeoutMs))
            break;
        QThread::msleep(kRetryDelayMs);
    }
    if (socket.state() != QLocalSocket::ConnectedState)
    {
        qWarning("SyncLens is already running but did not answer on %s", qPrintable(m_socketPath));
        return;
    }

    // The launcher's token lets the running copy's window take focus on Wayland.
    QByteArray message = kActivate.toByteArray();
    const QByteArray token = qgetenv("XDG_ACTIVATION_TOKEN");
    if (!token.isEmpty())
        message += ' ' + token;
    socket.write(message + '\n');
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
}
