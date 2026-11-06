#pragma once

#include <QFileSystemWatcher>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

struct ThemeInfo
{
    QString id;   // file name without .json; stored in settings
    QString name; // display name from the file
    bool user = false;
};

// Finds WebUI themes and builds the script that applies the selected one.
//
// Themes are JSON palettes (see resources/theme/theme-engine.js). Bundled
// themes live in the binary; files in ~/.config/SyncLens/themes/ add to them
// or replace a bundled theme with the same id. ~/.config/SyncLens/user.css,
// if present, is applied on top of any theme. Edits to either are picked up
// while SyncLens runs.
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);

    static QList<ThemeInfo> available();
    static QString userThemesDir();
    static QString userCssPath();

    // Script for the given theme id ("" = Resilio's own colours). Always safe
    // to run: it also undoes a previously applied theme.
    static QString script(const QString &themeId);

signals:
    // A user theme file or user.css changed on disk.
    void filesChanged();

private:
    void rewatch();

    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
