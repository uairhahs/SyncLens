#include "CredentialStore.h"

#include <QSettings>
#include <QCryptographicHash>

// Returns a safe QSettings key fragment for a URL by hashing it.
// Using the URL directly is unsafe because '/' is treated as a group separator.
static QString urlSettingsKey(const QString &url)
{
  return QString::fromLatin1(
      QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex());
}

// ── libsecret backend ────────────────────────────────────────────────────────
#ifdef HAS_LIBSECRET

// Qt defines 'signals' as a keyword-macro; GLib/GDBus also uses 'signals' as
// a struct member name, causing a compile error. Temporarily suppress it.
#pragma push_macro("signals")
#undef signals
extern "C"
{
#include <libsecret/secret.h>
}
#pragma pop_macro("signals")

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
  // Username is not sensitive — store in QSettings keyed by URL hash.
  // Using a hash avoids QSettings group-separator conflicts with '/' in URLs.
  QSettings s("SyncLens", "SyncLens");
  s.setValue("auth/" + urlSettingsKey(url) + "/username", username);

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
  // Look up the URL-keyed entry; fall back to the legacy global key for
  // migration from single-instance installs.
  const QString key = "auth/" + urlSettingsKey(url) + "/username";
  QString username = s.value(key).toString();
  if (username.isEmpty())
    username = s.value("auth/username").toString(); // legacy migration
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
  s.remove("auth/" + urlSettingsKey(url) + "/username");
  s.remove("auth/username"); // remove legacy global key if present

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

void CredentialStore::save(const QString &url, const QString &username,
                           const QString &)
{
  // Only the username is persisted; the user re-enters the password each
  // launch. The dialog is at least pre-filled, which improves UX.
  QSettings s("SyncLens", "SyncLens");
  s.setValue("auth/" + urlSettingsKey(url) + "/username", username);
}

Credential CredentialStore::load(const QString &url)
{
  QSettings s("SyncLens", "SyncLens");
  const QString key = "auth/" + urlSettingsKey(url) + "/username";
  QString username = s.value(key).toString();
  if (username.isEmpty())
    username = s.value("auth/username").toString(); // legacy migration
  Credential c;
  c.username = username;
  // valid stays false — caller must show dialog but can pre-fill username.
  return c;
}

void CredentialStore::clear(const QString &url)
{
  QSettings s("SyncLens", "SyncLens");
  s.remove("auth/" + urlSettingsKey(url) + "/username");
  s.remove("auth/username"); // remove legacy global key if present
}

#endif
