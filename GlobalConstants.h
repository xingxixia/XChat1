#pragma once

#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
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

inline QString LegacyDocumentsDataRootPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2");
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
    CopyMissingFilesRecursively(LegacyDocumentsDataRootPath(),
                                RuntimeDataRootPath());
    CopyMissingFilesRecursively(DefaultRuntimeDataRootPath(),
                                RuntimeDataRootPath());
}

inline QString UnselectedCharacterName()
{
    return QString::fromUtf8(QByteArray::fromHex("e69caae98089e68ba9"));
}

class RuntimePath
{
  public:
    enum Kind
    {
        JsonSetting,
        IniSetting,
        CharacterAssets,
        CharacterUserConfig,
        AnimePlugin
    };

    explicit constexpr RuntimePath(Kind kind) : m_kind(kind) {}

    QString toString() const
    {
        const QDir root(RuntimeDataRootPath());
        switch (m_kind)
        {
        case JsonSetting:
            return root.filePath("config.json");
        case IniSetting:
            return root.filePath("config.ini");
        case CharacterAssets:
            return root.filePath("Character/Assets");
        case CharacterUserConfig:
            return root.filePath("Character/UserConfig");
        case AnimePlugin:
            return root.filePath("Plugin/Anime");
        }
        return RuntimeDataRootPath();
    }

    operator QString() const { return toString(); }

  private:
    Kind m_kind;
};

inline QString operator+(const RuntimePath &path, const QString &suffix)
{
    return path.toString() + suffix;
}

inline QString operator+(const RuntimePath &path, const char *suffix)
{
    return path.toString() + QString::fromUtf8(suffix);
}

// Main portable configuration, such as API keys.
inline constexpr RuntimePath JsonSettingPath(RuntimePath::JsonSetting);

// Machine-local configuration, such as selected character and position.
inline constexpr RuntimePath IniSettingPath(RuntimePath::IniSetting);

// Character asset directory.
inline constexpr RuntimePath CharacterAssestPath(RuntimePath::CharacterAssets);

// Character user configuration directory.
inline constexpr RuntimePath CharacterUserConfigPath(
    RuntimePath::CharacterUserConfig);

// Animation plugin directory.
inline constexpr RuntimePath AnimePluginPath(RuntimePath::AnimePlugin);

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
