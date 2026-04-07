#include "HealthMonitor.h"

#include <QNetworkRequest>
#include <QUrl>

HealthMonitor::HealthMonitor(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &HealthMonitor::probe);
    connect(&m_nam, &QNetworkAccessManager::finished,
            this, &HealthMonitor::onReplyFinished);
    // If the WebUI requires auth, do not supply credentials in the probe.
    // An auth challenge (401) means the daemon is running.
    connect(&m_nam, &QNetworkAccessManager::authenticationRequired,
            this, [](QNetworkReply *, QAuthenticator *) {
        // Intentionally empty: leaving credentials blank causes Qt to fail
        // the reply immediately with AuthenticationRequiredError.
    });
}

void HealthMonitor::setUrl(const QString &url)
{
    m_url = url;
}

void HealthMonitor::setIntervalMs(int ms)
{
    m_timer.setInterval(ms);
}

void HealthMonitor::start()
{
    probe();
    m_timer.start();
}

void HealthMonitor::stop()
{
    m_timer.stop();
}

void HealthMonitor::probe()
{
    if (m_probeInFlight || m_url.isEmpty())
        return;

    m_probeInFlight = true;
    QUrl url(m_url);
    QNetworkRequest request(url);
    request.setTransferTimeout(5000);
    m_nam.get(request);
}

void HealthMonitor::onReplyFinished(QNetworkReply *reply)
{
    m_probeInFlight = false;
    reply->deleteLater();

    const auto err = reply->error();

    if (err == QNetworkReply::NoError ||
        err == QNetworkReply::AuthenticationRequiredError)
    {
        // NoError = WebUI is open and responded 2xx/3xx.
        // AuthenticationRequiredError = server sent 401; daemon is up.
        setState(HealthState::Running);
    } else if (err == QNetworkReply::ConnectionRefusedError) {
        setState(HealthState::Stopped);
    } else {
        setState(HealthState::Unreachable);
    }
}

void HealthMonitor::setState(HealthState s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(s);
}
