#pragma once

#include <QString>

struct Credential {
    QString username;
    QString password;
    bool    valid = false;
};

// Secure credential storage backed by the freedesktop Secrets Service
// (GNOME Keyring / KWallet) when built with libsecret. Falls back to
// username-only persistence via QSettings so the login dialog is at least
// pre-filled on subsequent launches even without the secret backend.
class CredentialStore
{
public:
    // Returns true if a secret-service backend is available.
    static bool backendAvailable();

    // Save username + password associated with the given service URL.
    static void save(const QString &url, const QString &username,
                     const QString &password);

    // Load stored credentials for the given service URL.
    static Credential load(const QString &url);

    // Remove stored credentials for the given service URL.
    static void clear(const QString &url);
};
