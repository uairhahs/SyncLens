#include "QuitOnSignal.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
// [0] is written by the signal handler, [1] is read by the event loop.
int g_fds[2] = {-1, -1};

void onSignal(int)
{
    // Only async-signal-safe calls here: the event loop does the real work.
    const char byte = 1;
    [[maybe_unused]] const ssize_t n = ::write(g_fds[0], &byte, 1);
}
} // namespace

QuitOnSignal::QuitOnSignal(QObject *parent)
    : QObject(parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, g_fds) != 0)
    {
        qWarning("SyncLens: cannot watch for SIGTERM; a stop request will end it abruptly");
        return;
    }

    m_notifier = new QSocketNotifier(g_fds[1], QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, []()
            {
        char byte;
        [[maybe_unused]] const ssize_t n = ::read(g_fds[1], &byte, 1);
        QCoreApplication::quit(); });

    struct sigaction sa = {};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

QuitOnSignal::~QuitOnSignal()
{
    if (g_fds[0] < 0)
        return;
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGINT, SIG_DFL);
    delete m_notifier;
    m_notifier = nullptr;
    ::close(g_fds[0]);
    ::close(g_fds[1]);
    g_fds[0] = g_fds[1] = -1;
}
