#include "ThemeManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QStandardPaths>
#include <QtDebug>

namespace
{
QString configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
           QStringLiteral("/SyncLens");
}

QByteArray readFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// A usable theme has a ramp of at least two greys and one accent.
QJsonObject loadTheme(const QString &path)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(readFile(path), &err);
    if (!doc.isObject())
    {
        qWarning("SyncLens: theme %s is not valid JSON: %s", qPrintable(path),
                 qPrintable(err.errorString()));
        return {};
    }
    const QJsonObject obj = doc.object();
    if (obj.value("greys").toArray().size() < 2 || obj.value("accents").toArray().isEmpty())
    {
        qWarning("SyncLens: theme %s needs \"greys\" (2 or more) and \"accents\"",
                 qPrintable(path));
        return {};
    }
    return obj;
}

// id -> path, user themes replacing bundled ones of the same id.
QMap<QString, QString> themePaths()
{
    QMap<QString, QString> paths;
    for (const QString &dir : {QStringLiteral(":/themes"), ThemeManager::userThemesDir()})
    {
        const QFileInfoList files = QDir(dir).entryInfoList({"*.json"}, QDir::Files);
        for (const QFileInfo &fi : files)
            paths.insert(fi.completeBaseName(), fi.filePath());
    }
    return paths;
}
} // namespace

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    // Saves often arrive as several events (write, rename); react once.
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(300);
    connect(&m_debounce, &QTimer::timeout, this, [this]()
            {
        rewatch();
        emit filesChanged(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce, qOverload<>(&QTimer::start));
    rewatch();
}

void ThemeManager::rewatch()
{
    // Editors often replace files rather than write them, which drops the
    // watch, so rebuild the list each time. The config dir is watched so
    // that creating user.css or themes/ is noticed.
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());

    QStringList paths;
    for (const QString &p : {configDir(), userThemesDir(), userCssPath()})
        if (QFileInfo::exists(p))
            paths << p;
    const QFileInfoList themes = QDir(userThemesDir()).entryInfoList({"*.json"}, QDir::Files);
    for (const QFileInfo &fi : themes)
        paths << fi.filePath();
    if (!paths.isEmpty())
        m_watcher.addPaths(paths);
}

QString ThemeManager::userThemesDir()
{
    return configDir() + QStringLiteral("/themes");
}

QString ThemeManager::userCssPath()
{
    return configDir() + QStringLiteral("/user.css");
}

QList<ThemeInfo> ThemeManager::available()
{
    QList<ThemeInfo> list;
    const QMap<QString, QString> paths = themePaths();
    for (auto it = paths.cbegin(); it != paths.cend(); ++it)
    {
        const QJsonObject obj = loadTheme(it.value());
        if (obj.isEmpty())
            continue;
        ThemeInfo info;
        info.id = it.key();
        info.name = obj.value("name").toString(it.key());
        info.user = !it.value().startsWith(':');
        list.append(info);
    }
    std::sort(list.begin(), list.end(), [](const ThemeInfo &a, const ThemeInfo &b)
              { return a.name.localeAwareCompare(b.name) < 0; });
    return list;
}

QString ThemeManager::script(const QString &themeId)
{
    static const QString engine = QString::fromUtf8(readFile(":/theme/theme-engine.js"));

    QJsonValue theme = QJsonValue::Null;
    if (!themeId.isEmpty())
    {
        const QString path = themePaths().value(themeId);
        const QJsonObject obj = path.isEmpty() ? QJsonObject() : loadTheme(path);
        if (!obj.isEmpty())
            theme = obj;
    }

    // Pass user.css as a JSON string so its contents can't break the script.
    const QJsonArray args{theme, QString::fromUtf8(readFile(userCssPath()))};
    QString call = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
    call = call.mid(1, call.size() - 2); // strip [ ]

    return engine + QStringLiteral("\nwindow.__synclensTheme.apply(") + call + QStringLiteral(");\n");
}
