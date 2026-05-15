#pragma once

#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QString>

inline QString ProjectRootPath()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i)
    {
        if (QDir(dir.filePath("res/default_config/ZcChat2")).exists())
            return dir.absolutePath();
        if (!dir.cdUp())
            break;
    }

    return QCoreApplication::applicationDirPath();
}

inline QString RuntimeDataRootPath()
{
    return QDir(ProjectRootPath()).filePath("runtime_data/ZcChat2");
}

inline QString DefaultRuntimeDataRootPath()
{
    return QDir(ProjectRootPath()).filePath("res/default_config/ZcChat2");
}

inline void CopyMissingFilesRecursively(const QString &sourceRoot,
                                        const QString &targetRoot)
{
    QDir sourceDir(sourceRoot);
    if (!sourceDir.exists())
        return;

    QDir().mkpath(targetRoot);
    const QFileInfoList entries = sourceDir.entryInfoList(
        QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &entry : entries)
    {
        const QString targetPath = QDir(targetRoot).filePath(entry.fileName());
        if (entry.isDir())
        {
            CopyMissingFilesRecursively(entry.absoluteFilePath(), targetPath);
            continue;
        }
        if (!QFileInfo::exists(targetPath))
            QFile::copy(entry.absoluteFilePath(), targetPath);
    }
}

inline void InitializeRuntimeData()
{
    CopyMissingFilesRecursively(DefaultRuntimeDataRootPath(),
                                RuntimeDataRootPath());
}

inline QString UnselectedCharacterName()
{
    return QString::fromUtf8(QByteArray::fromHex("e69caae98089e68ba9"));
}

// Main portable configuration, such as API keys.
inline const QString JsonSettingPath =
    QDir(RuntimeDataRootPath()).filePath("config.json");

// Machine-local configuration, such as selected character and position.
inline const QString IniSettingPath =
    QDir(RuntimeDataRootPath()).filePath("config.ini");

// Character asset directory.
inline const QString CharacterAssestPath =
    QDir(RuntimeDataRootPath()).filePath("Character/Assets");

// Character user configuration directory.
inline const QString CharacterUserConfigPath =
    QDir(RuntimeDataRootPath()).filePath("Character/UserConfig");

// Animation plugin directory.
inline const QString AnimePluginPath =
    QDir(RuntimeDataRootPath()).filePath("Plugin/Anime");

inline QString ReadNowSelectChar()
{
    QSettings settings(IniSettingPath, QSettings::IniFormat);
    return settings.value("character/CharSelect", UnselectedCharacterName()).toString();
}

inline QString ReadCharacterTachiePath()
{
    const QString charName = ReadNowSelectChar();
    if (charName == UnselectedCharacterName())
        return QString();

    const QString tachiePath =
        QDir(CharacterAssestPath).filePath(charName + "/Tachie");
    return QDir(tachiePath).exists() ? tachiePath : QString();
}

inline QString ReadCharacterUserConfigPath()
{
    const QString charName = ReadNowSelectChar();
    if (charName == UnselectedCharacterName())
        return QString();

    return QDir(CharacterUserConfigPath).filePath(charName + "/config.json");
}

inline QString ReadCharacterContextPath()
{
    const QString charName = ReadNowSelectChar();
    if (charName == UnselectedCharacterName())
        return QString();

    return QDir(CharacterUserConfigPath).filePath(charName + "/context.json");
}
