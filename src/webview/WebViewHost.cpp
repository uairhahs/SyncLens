#include "WebViewHost.h"
#include "settings/CredentialStore.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
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

WebViewHost::WebViewHost(QWidget *parent)
    : QWidget(parent)
{
    // Persistent profile: stores cookies in
    // ~/.local/share/<OrgName>/<AppName>/QtWebEngine/synclens/
    // Cookies survive process exit and are reloaded on next launch — the user
    // is not prompted to sign in again as long as the Resilio session cookie
    // is still valid.
    m_profile = new QWebEngineProfile("synclens", this);
    m_profile->setPersistentCookiesPolicy(
        QWebEngineProfile::ForcePersistentCookies);

    // Page is parented to the profile so the profile outlives the page.
    // The view uses setPage() so it does NOT take ownership.
    // Destruction order: view destroyed first (child of m_stack/this),
    // then profile destroyed (direct child of this, created before m_stack),
    // which destroys the page as its own child. This is the correct order.
    m_page = new QWebEnginePage(m_profile, m_profile);

    m_stack = new QStackedWidget(this);

    // --- Embedded web view ---
    m_webView = new QWebEngineView(m_stack);
    m_webView->setPage(m_page); // view does not own the page
    m_stack->addWidget(m_webView);

    // --- Offline / error page ---
    m_offlinePage = new QWidget(m_stack);
    auto *offlineLayout = new QVBoxLayout(m_offlinePage);
    offlineLayout->setAlignment(Qt::AlignCenter);

    m_offlineLabel = new QLabel(m_offlinePage);
    m_offlineLabel->setAlignment(Qt::AlignCenter);
    m_offlineLabel->setWordWrap(true);

    m_openBrowserButton = new QPushButton(tr("Open in Browser"), m_offlinePage);
    connect(m_openBrowserButton, &QPushButton::clicked, this, [this]() {
        emit openInBrowserRequested(m_currentUrl);
    });

    offlineLayout->addWidget(m_offlineLabel);
    offlineLayout->addSpacing(12);
    offlineLayout->addWidget(m_openBrowserButton);
    m_stack->addWidget(m_offlinePage);

    connect(m_page, &QWebEnginePage::authenticationRequired,
            this, &WebViewHost::onAuthRequired);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_stack);
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
    if (m_authAttempt == 0) {
        ++m_authAttempt;
        Credential stored = CredentialStore::load(urlKey);
        if (stored.valid) {
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
    if (CredentialStore::backendAvailable()) {
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

    if (dlg.exec() == QDialog::Accepted) {
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
