#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

// Asks GitHub for the latest SyncLens release and compares it with this build.
class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check();
    bool isChecking() const { return m_inFlight; }

    static QString currentVersion();

signals:
    // `version` is the release's version without the leading "v".
    void updateAvailable(const QString &version, const QString &releaseUrl);
    void upToDate(const QString &latestVersion);
    void failed(const QString &error);

private:
    QNetworkAccessManager m_nam;
    bool m_inFlight = false;
};
