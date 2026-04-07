#pragma once

#include "HealthState.h"
#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class HealthMonitor : public QObject
{
    Q_OBJECT
public:
    explicit HealthMonitor(QObject *parent = nullptr);

    void setUrl(const QString &url);
    void setIntervalMs(int ms);
    void start();
    void stop();

    HealthState state() const { return m_state; }

signals:
    void stateChanged(HealthState newState);

private slots:
    void probe();
    void onReplyFinished(QNetworkReply *reply);

private:
    void setState(HealthState s);

    QTimer m_timer;
    QNetworkAccessManager m_nam;
    QString m_url;
    HealthState m_state = HealthState::Unknown;
    bool m_probeInFlight = false;
};
