#include "CredentialStore.h"

#include <QSettings>

// ── libsecret backend ────────────────────────────────────────────────────────
#ifdef HAS_LIBSECRET

extern "C"
{
#include <libsecret/secret.h>
}

// One schema, keyed by the WebUI URL string.
static const SecretSchema kSchema = {
    "io.synclens.webui-credential",
    SECRET_SCHEMA_NONE,
    {{"url", SECRET_SCHEMA_ATTRIBUTE_STRING},
     {nullptr, static_cast<SecretSchemaAttributeType>(0)}},
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr};

bool CredentialStore::backendAvailable() { return true; }

void CredentialStore::save(const QString &url, const QString &username,
                           const QString &password)
{
  // Username is not sensitive — store in QSettings for the fallback path too.
  QSettings s("SyncLens", "SyncLens");
  s.setValue("auth/username", username);

  const QString label =
      QString("SyncLens WebUI credentials (%1)").arg(url);

  GError *error = nullptr;
  secret_password_store_sync(
      &kSchema,
      SECRET_COLLECTION_DEFAULT,
      label.toUtf8().constData(),
      password.toUtf8().constData(),
      nullptr, // GCancellable
      &error,
      "url", url.toUtf8().constData(),
      nullptr // attribute sentinel
  );
  if (error)
    g_error_free(error);
}

Credential CredentialStore::load(const QString &url)
{
  QSettings s("SyncLens", "SyncLens");
  const QString username = s.value("auth/username").toString();
  if (username.isEmpty())
    return {};

  GError *error = nullptr;
  gchar *pw = secret_password_lookup_sync(
      &kSchema,
      nullptr, // GCancellable
      &error,
      "url", url.toUtf8().constData(),
      nullptr);

  if (error)
  {
    g_error_free(error);
    return {};
  }
  if (!pw)
    return {};

  Credential cred;
  cred.username = username;
  cred.password = QString::fromUtf8(pw);
  cred.valid = true;
  secret_password_free(pw);
  return cred;
}

void CredentialStore::clear(const QString &url)
{
  QSettings s("SyncLens", "SyncLens");
  s.remove("auth/username");

  GError *error = nullptr;
  secret_password_clear_sync(
      &kSchema,
      nullptr,
      &error,
      "url", url.toUtf8().constData(),
      nullptr);
  if (error)
    g_error_free(error);
}

// ── fallback (no libsecret) ──────────────────────────────────────────────────
#else

bool CredentialStore::backendAvailable() { return false; }

void CredentialStore::save(const QString &, const QString &username,
                           const QString &)
{
  // Only the username is persisted; the user re-enters the password each
  // launch. The dialog is at least pre-filled, which improves UX.
  QSettings s("SyncLens", "SyncLens");
  s.setValue("auth/username", username);
}

Credential CredentialStore::load(const QString &)
{
  QSettings s("SyncLens", "SyncLens");
  Credential c;
  c.username = s.value("auth/username").toString();
  // valid stays false — caller must show dialog but can pre-fill username.
  return c;
}

void CredentialStore::clear(const QString &)
{
  QSettings s("SyncLens", "SyncLens");
  s.remove("auth/username");
}

#endif
