#pragma once

#include <QWidget>
#include <QString>
#include <QWebEnginePage>

QT_BEGIN_NAMESPACE
class QWebEngineView;
class QWebEngineProfile;
class QStackedWidget;
class QLabel;
class QPushButton;
class QAuthenticator;
class QUrl;
QT_END_NAMESPACE

class CredentialStore;

class WebViewHost : public QWidget
{
    Q_OBJECT
public:
    // profileId is used as the QWebEngineProfile storage name, giving each
    // Sync instance its own isolated cookie jar and session data.
    explicit WebViewHost(const QString &profileId, QWidget *parent = nullptr);

    void loadUrl(const QString &url);
    void setUrl(const QString &url);
    void showOfflinePage(const QString &message);
    void showWebView();

    // Replace the page with a fresh one (new renderer process) and reload.
    void reconnect();

    // Script injected into every load of this view's profile, and run once on
    // the current page so changes apply without a reload.
    void setThemeScript(const QString &source);

signals:
    void reconnectRequested();

private:
    void onAuthRequired(const QUrl &requestUrl, QAuthenticator *authenticator);
    // Keychain key for a challenge: the instance URL when the request is on
    // the instance's origin, so every sub-resource maps to one entry.
    QString credentialKey(const QUrl &requestUrl) const;
    void onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status, int exitCode);
    void createPage();

    // Profile must outlive the page and view. It is declared first so it is
    // destroyed last (Qt child destruction is reverse-construction order).
    QWebEngineProfile *m_profile;
    QWebEnginePage *m_page;

    QStackedWidget *m_stack;
    QWebEngineView *m_webView;
    QWidget *m_offlinePage;
    QLabel *m_offlineLabel;
    QPushButton *m_reconnectButton;
    QString m_currentUrl;

    // Per main-frame navigation: stored credentials are offered once, then
    // the user is asked; after a cancel, further challenges just fail.
    bool m_triedStored = false;
    bool m_authCancelled = false;
};
