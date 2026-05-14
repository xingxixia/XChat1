#include "settingchild_text.h"

#include "../../../GlobalConstants.h"

#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ZcJsonLib.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
struct TextFontPreset
{
    const char *id;
    const char *name;
    QStringList fontCandidates;
};

QList<TextFontPreset> textFontPresets()
{
    return {
        {"clear", "标准对白",
         {QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"),
          QStringLiteral("微软雅黑")}},
        {"soft", "柔和圆体",
         {QStringLiteral("Microsoft JhengHei UI"),
          QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei")}},
        {"mono", "等宽阅读",
         {QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"),
          QStringLiteral("Microsoft YaHei UI")}},
    };
}

QString resolvePresetFontFamily(const QString &presetId)
{
    const QStringList families = QFontDatabase::families();
    for (const TextFontPreset &preset : textFontPresets())
    {
        if (presetId != QString::fromLatin1(preset.id))
            continue;
        for (const QString &candidate : preset.fontCandidates)
        {
            if (families.contains(candidate))
                return candidate;
        }
    }
    return QStringLiteral("Microsoft YaHei UI");
}
} // namespace

SettingChild_Text::SettingChild_Text(QWidget *parent)
    : QWidget(parent)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(30, 30, 30, 30);
    rootLayout->setSpacing(16);

    auto *title = new ElaText(this);
    title->setText(QStringLiteral("文本设置"));
    title->setTextPixelSize(25);
    rootLayout->addWidget(title);

    auto *fontRow = new ElaScrollPageArea(this);
    auto *fontLayout = new QHBoxLayout(fontRow);
    fontLayout->setContentsMargins(15, 0, 15, 0);
    auto *fontLabel = new ElaText(fontRow);
    fontLabel->setText(QStringLiteral("字体方案"));
    fontLabel->setMinimumWidth(120);
    m_fontPresetComboBox = new QComboBox(fontRow);
    for (const TextFontPreset &preset : textFontPresets())
    {
        m_fontPresetComboBox->addItem(QString::fromUtf8(preset.name),
                                      QString::fromLatin1(preset.id));
    }
    fontLayout->addWidget(fontLabel);
    fontLayout->addWidget(m_fontPresetComboBox, 1);
    rootLayout->addWidget(fontRow);

    auto *boxRow = new ElaScrollPageArea(this);
    auto *boxLayout = new QHBoxLayout(boxRow);
    boxLayout->setContentsMargins(15, 0, 15, 0);
    auto *boxLabel = new ElaText(boxRow);
    boxLabel->setText(QStringLiteral("文本框"));
    boxLabel->setMinimumWidth(120);
    m_textBoxThemeComboBox = new QComboBox(boxRow);
    m_textBoxThemeComboBox->addItem(QStringLiteral("浅色透明框"),
                                    QStringLiteral("light"));
    m_textBoxThemeComboBox->addItem(QStringLiteral("深色透明框"),
                                    QStringLiteral("dark"));
    m_textBoxThemeComboBox->addItem(QStringLiteral("浅色对白框"),
                                    QStringLiteral("gal_light"));
    m_textBoxThemeComboBox->addItem(QStringLiteral("深色对白框"),
                                    QStringLiteral("gal_dark"));
    boxLayout->addWidget(boxLabel);
    boxLayout->addWidget(m_textBoxThemeComboBox, 1);
    rootLayout->addWidget(boxRow);

    auto *sizeRow = new ElaScrollPageArea(this);
    auto *sizeLayout = new QHBoxLayout(sizeRow);
    sizeLayout->setContentsMargins(15, 0, 15, 0);
    auto *sizeLabel = new ElaText(sizeRow);
    sizeLabel->setText(QStringLiteral("字号"));
    sizeLabel->setMinimumWidth(120);
    m_fontSizeSpinBox = new QSpinBox(sizeRow);
    m_fontSizeSpinBox->setRange(8, 32);
    m_fontSizeSpinBox->setSuffix(QStringLiteral(" pt"));
    sizeLayout->addWidget(sizeLabel);
    sizeLayout->addWidget(m_fontSizeSpinBox, 1);
    rootLayout->addWidget(sizeRow);

    rootLayout->addStretch();

    loadConfig();

    connect(m_fontPresetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int)
            {
                saveConfig();
            });
    connect(m_textBoxThemeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int)
            {
                saveConfig();
            });
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [this](int)
            {
                saveConfig();
            });
}

void SettingChild_Text::loadConfig()
{
    m_loading = true;
    const QSignalBlocker fontBlocker(m_fontPresetComboBox);
    const QSignalBlocker boxBlocker(m_textBoxThemeComboBox);
    const QSignalBlocker sizeBlocker(m_fontSizeSpinBox);

    ZcJsonLib config(JsonSettingPath);
    const QString presetId =
        config.value("text/fontPreset").toString().trimmed();
    const QString boxTheme =
        config.value("text/textBoxTheme").toString().trimmed();
    const int fontSize = config.value("text/fontSize").toInt();

    const int presetIndex =
        m_fontPresetComboBox->findData(presetId.isEmpty()
                                           ? QStringLiteral("clear")
                                           : presetId);
    m_fontPresetComboBox->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
    const int boxIndex =
        m_textBoxThemeComboBox->findData(boxTheme.isEmpty()
                                             ? QStringLiteral("light")
                                             : boxTheme);
    m_textBoxThemeComboBox->setCurrentIndex(boxIndex >= 0 ? boxIndex : 0);
    m_fontSizeSpinBox->setValue(fontSize > 0 ? fontSize : 12);

    m_loading = false;
}

void SettingChild_Text::saveConfig()
{
    if (m_loading)
        return;

    ZcJsonLib config(JsonSettingPath);
    const QString presetId = m_fontPresetComboBox->currentData().toString();
    config.setValue("text/fontPreset", presetId);
    config.setValue("text/fontFamily", resolvePresetFontFamily(presetId));
    config.setValue("text/textBoxTheme",
                    m_textBoxThemeComboBox->currentData().toString());
    config.setValue("text/fontSize", m_fontSizeSpinBox->value());

    emit textSettingsChanged();
}
