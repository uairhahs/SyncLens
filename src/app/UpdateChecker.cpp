#include "UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVersionNumber>

static constexpr const char *kLatestReleaseApi =
    "https://api.github.com/repos/uairhahs/SyncLens/releases/latest";

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
}

QString UpdateChecker::currentVersion()
{
    return QStringLiteral(SYNCLENS_VERSION);
}

void UpdateChecker::check()
{
    if (m_inFlight)
        return;
    m_inFlight = true;

    QNetworkRequest request{QUrl(kLatestReleaseApi)};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("SyncLens/") + currentVersion());
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
            {
        m_inFlight = false;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            emit failed(reply->errorString());
            return;
        }

        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        QString tag = release.value("tag_name").toString();
        if (tag.startsWith('v'))
            tag.remove(0, 1);
        const QVersionNumber latest = QVersionNumber::fromString(tag);
        if (latest.isNull())
        {
            emit failed(tr("GitHub returned no release version."));
            return;
        }

        if (latest > QVersionNumber::fromString(currentVersion()))
            emit updateAvailable(tag, release.value("html_url").toString());
        else
            emit upToDate(tag); });
}
