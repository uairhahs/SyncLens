#include "WebViewHost.h"
#include "settings/CredentialStore.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QMetaObject>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDialog>
#include <QAuthenticator>
#include <QUrl>
#include <QFrame>
#include <QCheckBox>

WebViewHost::WebViewHost(const QString &profileId, QWidget *parent)
    : QWidget(parent)
{
    // ── Destruction-order guarantee ──────────────────────────────────────────
    // Qt deletes children in FIFO order (first added → first deleted).
    // Required order for QtWebEngine: view first, page second, profile last.
    // We achieve this by adding m_stack (parent of the view) to the children
    // list BEFORE m_profile (parent of the page).  Profile is created last so
    // it sits later in the children list and is therefore deleted last.
    // ─────────────────────────────────────────────────────────────────────────

    // 1. Stack and view — must be destroyed FIRST.
    m_stack = new QStackedWidget(this); // children[0]

    m_webView = new QWebEngineView(m_stack);

    // Offline / error page (also a stack child — destroyed together with m_stack).
    m_offlinePage = new QWidget(m_stack);
    auto *offlineLayout = new QVBoxLayout(m_offlinePage);
    offlineLayout->setAlignment(Qt::AlignCenter);

    m_offlineLabel = new QLabel(m_offlinePage);
    m_offlineLabel->setAlignment(Qt::AlignCenter);
    m_offlineLabel->setWordWrap(true);

    m_openBrowserButton = new QPushButton(tr("Open in Browser"), m_offlinePage);
    connect(m_openBrowserButton, &QPushButton::clicked, this, [this]()
            { emit openInBrowserRequested(m_currentUrl); });

    offlineLayout->addWidget(m_offlineLabel);
    offlineLayout->addSpacing(12);
    offlineLayout->addWidget(m_openBrowserButton);

    m_stack->addWidget(m_webView);
    m_stack->addWidget(m_offlinePage);

    // 2. Profile and page — must be destroyed LAST.
    // Profile stores cookies in:
    //   ~/.local/share/<OrgName>/<AppName>/QtWebEngine/<profileId>/
    // Each Sync instance gets its own profile, so sessions are fully isolated.
    m_profile = new QWebEngineProfile(profileId, this); // children[1]
    m_profile->setPersistentCookiesPolicy(
        QWebEngineProfile::ForcePersistentCookies);

    createPage();

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_stack);
}

void WebViewHost::createPage()
{
    // Page is parented to the profile; the view uses setPage() and does NOT
    // take ownership.  When profile is eventually destroyed it deletes the page.
    // A new page gets a new renderer process, which is what reconnect() needs.
    m_page = new QWebEnginePage(m_profile, m_profile);
    m_webView->setPage(m_page);

    connect(m_page, &QWebEnginePage::authenticationRequired,
            this, &WebViewHost::onAuthRequired);
    connect(m_page, &QWebEnginePage::renderProcessTerminated,
            this, &WebViewHost::onRenderProcessTerminated);
}

void WebViewHost::reconnect()
{
    QWebEnginePage *old = m_page;
    createPage();
    // Detached from the view by setPage(); delete once Qt is done with it.
    old->deleteLater();
    if (!m_currentUrl.isEmpty())
        loadUrl(m_currentUrl);
}

void WebViewHost::onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status,
                                             int exitCode)
{
    Q_UNUSED(exitCode);
    // Normal termination happens while a page is being replaced; anything
    // else (crash, kill, OOM) leaves a blank view, so start a fresh renderer.
    if (status != QWebEnginePage::NormalTerminationStatus)
        QMetaObject::invokeMethod(this, &WebViewHost::reconnect, Qt::QueuedConnection);
}

void WebViewHost::loadUrl(const QString &url)
{
    m_authAttempt = 0; // reset on every new navigation
    m_currentUrl = url;
    m_webView->setUrl(QUrl(url));
}

void WebViewHost::setUrl(const QString &url)
{
    m_currentUrl = url;
}

void WebViewHost::showOfflinePage(const QString &message)
{
    m_offlineLabel->setText(message);
    m_stack->setCurrentWidget(m_offlinePage);
}

void WebViewHost::showWebView()
{
    m_stack->setCurrentWidget(m_webView);
}

void WebViewHost::onAuthRequired(const QUrl &requestUrl, QAuthenticator *authenticator)
{
    const QString urlKey = requestUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);

    // First attempt: try stored credentials silently without showing a dialog.
    if (m_authAttempt == 0)
    {
        ++m_authAttempt;
        Credential stored = CredentialStore::load(urlKey);
        if (stored.valid)
        {
            authenticator->setUser(stored.username);
            authenticator->setPassword(stored.password);
            return; // Qt will retry the request with these credentials.
        }
        // No stored credentials — fall through to the dialog below.
    }

    // Second attempt onward: stored credentials were wrong or absent.
    // Clear any stale entry and show the login dialog.
    if (m_authAttempt > 1)
        CredentialStore::clear(urlKey);
    ++m_authAttempt;

    Credential hint = CredentialStore::load(urlKey); // username hint even in fallback mode

    // ── Dialog ───────────────────────────────────────────────────────────────
    QDialog dlg(window());
    dlg.setWindowTitle("SyncLens");
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setMinimumWidth(360);

    auto *root = new QVBoxLayout(&dlg);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // Header banner
    auto *header = new QWidget(&dlg);
    header->setStyleSheet("background: #1a1a2e; padding: 0;");
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(24, 20, 24, 20);
    headerLayout->setSpacing(4);

    auto *appLabel = new QLabel("SyncLens", header);
    appLabel->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold;"
                            " background: transparent;");
    auto *hostLabel = new QLabel(tr("Connecting to %1").arg(requestUrl.host()), header);
    hostLabel->setStyleSheet("color: #8888aa; font-size: 12px; background: transparent;");

    headerLayout->addWidget(appLabel);
    headerLayout->addWidget(hostLabel);
    root->addWidget(header);

    // Form
    auto *body = new QWidget(&dlg);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(24, 20, 24, 20);
    bodyLayout->setSpacing(12);

    auto *userEdit = new QLineEdit(body);
    userEdit->setPlaceholderText(tr("Username"));
    userEdit->setMinimumHeight(36);
    if (!hint.username.isEmpty())
        userEdit->setText(hint.username);

    auto *passEdit = new QLineEdit(body);
    passEdit->setPlaceholderText(tr("Password"));
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setMinimumHeight(36);

    // Only show "Remember" checkbox when a secret backend is available.
    QCheckBox *rememberCheck = nullptr;
    if (CredentialStore::backendAvailable())
    {
        rememberCheck = new QCheckBox(tr("Remember password"), body);
        rememberCheck->setChecked(true);
    }

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, body);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Sign In"));
    buttons->button(QDialogButtonBox::Ok)->setMinimumHeight(34);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    connect(userEdit, &QLineEdit::returnPressed, passEdit,
            QOverload<>::of(&QWidget::setFocus));
    connect(passEdit, &QLineEdit::returnPressed, &dlg, &QDialog::accept);

    bodyLayout->addWidget(userEdit);
    bodyLayout->addWidget(passEdit);
    if (rememberCheck)
        bodyLayout->addWidget(rememberCheck);
    bodyLayout->addSpacing(4);
    bodyLayout->addWidget(buttons);
    root->addWidget(body);

    (hint.username.isEmpty() ? userEdit : passEdit)->setFocus();

    if (dlg.exec() == QDialog::Accepted)
    {
        const QString user = userEdit->text();
        const QString pass = passEdit->text();
        authenticator->setUser(user);
        authenticator->setPassword(pass);

        const bool shouldSave = !rememberCheck || rememberCheck->isChecked();
        if (shouldSave)
            CredentialStore::save(urlKey, user, pass);
        else
            CredentialStore::clear(urlKey);
    }
    // If cancelled: authenticator stays empty → single ERR_FAILED, no loop.
}
