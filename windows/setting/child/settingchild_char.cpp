#include "settingchild_char.h"
#include "ui_settingchild_char.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include "ElaComboBox.h"
#include "ElaMessageBar.h"
#include "ElaPlainTextEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

static QStringList ReadLlmModelList(const QString &serverSelect)
{
    ZcJsonLib config(JsonSettingPath);
    QStringList modelList;
    const QJsonArray modelArray =
        config.value("llm/" + serverSelect + "/ModelList").toArray();
    for (const QJsonValue &model : modelArray)
        modelList.append(model.toString());
    return modelList;
}

static QString DefaultPetPrompt()
{
    return QStringLiteral(
        "你是一个桌面陪伴 AI，像 Galgame 角色一样陪伴用户。"
        "你可以轻松、可爱、自然地聊天，也可以在用户需要时帮忙看代码、解释报错和整理思路。"
        "回复时优先使用中文，语气要像在桌面旁边陪着用户，而不是生硬的工具。");
}

static QString PrimaryCharacterName()
{
    return QStringLiteral("test");
}

static int TextWidthWithPadding(const QWidget *widget, const QString &text,
                                int minWidth, int padding)
{
    const int textWidth = widget->fontMetrics().horizontalAdvance(text);
    return qMax(minWidth, textWidth + padding);
}

/*初始化窗口*/
SettingChild_Char::SettingChild_Char(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Char)
{
    ui->setupUi(this);
    SetupPetPromptEditor();
    SetupPageSwitcher();
    SetupHistoryPage();
    //初始化动画管理器
    m_pluginManager.Reload();

    RefreshCharList();
    ui->BreadcrumbBar->appendBreadcrumb("角色设置");
    ui->BreadcrumbBar->setTextPixelSize(25);

    /*读取配置项*/
    //角色选择
    QSettings *settings =
        new QSettings(IniSettingPath, QSettings::IniFormat, this);
    settings->setValue("character/CharSelect", PrimaryCharacterName());
    QString defaultChar = PrimaryCharacterName();
    ui->comboBox_CharList->setCurrentText(defaultChar);
    ui->widget_SelectChar->hide();
    LoadCurrentCharConfig();

    isAlreadyLoading = true;
}

SettingChild_Char::~SettingChild_Char()
{
    delete ui;
}

void SettingChild_Char::SetupPetPromptEditor()
{
    auto *pageLayout = qobject_cast<QVBoxLayout *>(ui->page_Char->layout());
    if (!pageLayout)
        return;

    auto *area = new ElaScrollPageArea(ui->page_Char);
    auto *layout = new QHBoxLayout(area);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setStretch(0, 1);
    layout->setStretch(1, 1);

    auto *label = new ElaText(area);
    label->setText(QStringLiteral("桌宠行为设定"));
    QFont labelFont = label->font();
    labelFont.setPointSize(12);
    label->setFont(labelFont);

    m_petPromptEdit = new ElaPlainTextEdit(area);
    m_petPromptEdit->setMinimumHeight(90);
    m_petPromptEdit->setPlaceholderText(
        QStringLiteral("例如：你是一个温柔可爱的桌面陪伴 AI，会陪用户写代码、休息和聊天。"));
    m_petPromptEdit->setStyleSheet(ui->plainTextEdit_CharPrompt->styleSheet());

    layout->addWidget(label);
    layout->addWidget(m_petPromptEdit);

    const int insertIndex = pageLayout->indexOf(ui->widget_CharPrompt) + 1;
    pageLayout->insertWidget(insertIndex, area);

    connect(m_petPromptEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!isAlreadyLoading || !m_petPromptEdit)
            return;

        QString charName = ui->comboBox_CharList->currentText();
        if (charName.isEmpty() || charName == QStringLiteral("未选择"))
            return;

        ZcJsonLib charConfig(
            QDir(CharacterAssestPath).filePath(charName + "/config.json"));
        charConfig.setValue("petPrompt", m_petPromptEdit->toPlainText());
    });
}

void SettingChild_Char::SetupPageSwitcher()
{
    auto *rootLayout = qobject_cast<QVBoxLayout *>(layout());
    if (!rootLayout)
        return;

    auto *area = new ElaScrollPageArea(this);
    auto *layout = new QHBoxLayout(area);
    layout->setContentsMargins(15, 4, 15, 4);

    m_pageOneButton = new ElaPushButton(QStringLiteral("1"), area);
    m_pageTwoButton = new ElaPushButton(QStringLiteral("2"), area);
    m_pageOneButton->setMinimumWidth(TextWidthWithPadding(m_pageOneButton, QStringLiteral("1"), 46, 28));
    m_pageTwoButton->setMinimumWidth(TextWidthWithPadding(m_pageTwoButton, QStringLiteral("2"), 46, 28));

    layout->addStretch();
    layout->addWidget(m_pageOneButton);
    layout->addWidget(m_pageTwoButton);
    layout->addStretch();

    rootLayout->addWidget(area);

    connect(m_pageOneButton, &QPushButton::clicked, this,
            &SettingChild_Char::ShowPrimaryPage);
    connect(m_pageTwoButton, &QPushButton::clicked, this,
            &SettingChild_Char::ShowHistoryPage);
}

void SettingChild_Char::SetupHistoryPage()
{
    m_historyPage = new QWidget(ui->stackedWidget);
    auto *pageLayout = new QVBoxLayout(m_historyPage);
    pageLayout->setContentsMargins(15, 15, 15, 15);

    auto *toolbar = new ElaScrollPageArea(m_historyPage);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(15, 8, 15, 8);

    auto *refreshButton = new ElaPushButton(QStringLiteral("刷新"), toolbar);
    auto *clearButton = new ElaPushButton(QStringLiteral("清空全部"), toolbar);
    refreshButton->setMinimumWidth(TextWidthWithPadding(refreshButton, refreshButton->text(), 76, 34));
    clearButton->setMinimumWidth(TextWidthWithPadding(clearButton, clearButton->text(), 104, 34));
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addStretch();
    pageLayout->addWidget(toolbar);

    auto *scrollArea = new QScrollArea(m_historyPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { border: none; background: transparent; width: 8px; }"
        "QScrollBar::handle:vertical { background: rgba(136, 136, 136, 0.5); border-radius: 4px; }"));

    auto *container = new QWidget(scrollArea);
    m_historyListLayout = new QVBoxLayout(container);
    m_historyListLayout->setContentsMargins(0, 0, 0, 0);
    m_historyListLayout->addStretch();
    scrollArea->setWidget(container);
    pageLayout->addWidget(scrollArea);

    ui->stackedWidget->addWidget(m_historyPage);

    connect(refreshButton, &QPushButton::clicked, this,
            &SettingChild_Char::RefreshHistoryPage);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        SaveHistoryLines(QStringList());
        RefreshHistoryPage();
    });
}

void SettingChild_Char::SetPrimaryPageVisible(bool visible)
{
    ui->widget_CharPrompt->setVisible(visible);
    if (m_petPromptEdit)
    {
        if (QWidget *area = m_petPromptEdit->parentWidget())
            area->setVisible(visible);
    }
    ui->label_4->setVisible(visible);
    ui->widget_ApiKey_2->setVisible(visible);
    ui->label->setVisible(visible);
    ui->widget_Run_LLM->setVisible(visible);
    ui->widget_Run_Vits->setVisible(visible);
    ui->widget_Tachie_Set->setVisible(visible);
}

void SettingChild_Char::ShowPrimaryPage()
{
    SetPrimaryPageVisible(true);
    ui->stackedWidget->setCurrentIndex(0);
}

void SettingChild_Char::ShowHistoryPage()
{
    SetPrimaryPageVisible(false);
    RefreshHistoryPage();
    if (m_historyPage)
        ui->stackedWidget->setCurrentWidget(m_historyPage);
}

QStringList SettingChild_Char::ReadHistoryLines() const
{
    QStringList historyLines;
    const QString contextPath = ReadCharacterContextPath();
    if (contextPath.isEmpty())
        return historyLines;

    ZcJsonLib contextConfig(contextPath);
    const QJsonArray historyArray =
        contextConfig.value("history", QJsonValue(QJsonArray())).toArray();
    for (const QJsonValue &value : historyArray)
    {
        const QString line = value.toString();
        if (!line.isEmpty())
            historyLines.append(line);
    }
    return historyLines;
}

void SettingChild_Char::SaveHistoryLines(const QStringList &historyLines)
{
    const QString contextPath = ReadCharacterContextPath();
    if (contextPath.isEmpty())
        return;

    const QFileInfo fileInfo(contextPath);
    QDir().mkpath(fileInfo.absolutePath());

    QJsonArray historyArray;
    for (const QString &line : historyLines)
    {
        const QString trimmedLine = line.trimmed();
        if (!trimmedLine.isEmpty())
            historyArray.append(trimmedLine);
    }

    ZcJsonLib contextConfig(contextPath);
    contextConfig.setValue("history", QJsonValue(historyArray));
    emit requestReloadContextHistory();
}

void SettingChild_Char::ClearHistoryRows()
{
    for (QWidget *row : m_historyRows)
    {
        if (row)
            row->deleteLater();
    }
    m_historyRows.clear();
}

void SettingChild_Char::RefreshHistoryPage()
{
    if (!m_historyListLayout)
        return;

    ClearHistoryRows();
    const QStringList historyLines = ReadHistoryLines();
    const int insertIndex = qMax(0, m_historyListLayout->count() - 1);

    if (historyLines.isEmpty())
    {
        auto *emptyLabel = new ElaText(QStringLiteral("暂无历史对话"), m_historyPage);
        QFont font = emptyLabel->font();
        font.setPointSize(12);
        emptyLabel->setFont(font);
        m_historyListLayout->insertWidget(insertIndex, emptyLabel);
        m_historyRows.append(emptyLabel);
        return;
    }

    for (int i = 0; i < historyLines.size(); ++i)
    {
        const QString historyLine = historyLines.at(i);
        const bool isUserLine = historyLine.startsWith(QStringLiteral("用户："));
        const bool isRoleLine = historyLine.startsWith(QStringLiteral("角色："));
        const QString speakerText = isUserLine ? QStringLiteral("你")
                                               : (isRoleLine ? QStringLiteral("她")
                                                             : QStringLiteral("记录"));
        const QString rowStyle = isUserLine
                                     ? QStringLiteral("background: rgba(74, 144, 226, 26); border-radius: 8px;")
                                     : (isRoleLine
                                            ? QStringLiteral("background: rgba(236, 112, 153, 28); border-radius: 8px;")
                                            : QStringLiteral("background: rgba(136, 136, 136, 20); border-radius: 8px;"));
        const QString editStyle = ui->plainTextEdit_CharPrompt->styleSheet() +
                                  (isUserLine
                                       ? QStringLiteral("\nQPlainTextEdit { background: rgba(74, 144, 226, 34); }")
                                       : (isRoleLine
                                              ? QStringLiteral("\nQPlainTextEdit { background: rgba(236, 112, 153, 38); }")
                                              : QStringLiteral("\nQPlainTextEdit { background: rgba(136, 136, 136, 24); }")));

        auto *row = new ElaScrollPageArea(m_historyPage);
        row->setStyleSheet(rowStyle);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);

        auto *indexLabel = new ElaText(QString::number(i + 1), row);
        indexLabel->setMinimumWidth(
            TextWidthWithPadding(indexLabel, indexLabel->text(), 42, 20));

        auto *speakerLabel = new ElaText(speakerText, row);
        QFont speakerFont = speakerLabel->font();
        speakerFont.setBold(true);
        speakerLabel->setFont(speakerFont);
        speakerLabel->setAlignment(Qt::AlignCenter);
        speakerLabel->setMinimumWidth(
            TextWidthWithPadding(speakerLabel, speakerText, 58, 28));
        if (isUserLine)
            speakerLabel->setStyleSheet(QStringLiteral("color: rgb(48, 105, 180);"));
        else if (isRoleLine)
            speakerLabel->setStyleSheet(QStringLiteral("color: rgb(190, 76, 118);"));

        auto *edit = new ElaPlainTextEdit(row);
        edit->setPlainText(historyLine);
        edit->setMinimumHeight(58);
        edit->setStyleSheet(editStyle);

        auto *saveButton = new ElaPushButton(QStringLiteral("保存"), row);
        auto *deleteButton = new ElaPushButton(QStringLiteral("删除"), row);
        auto *deleteRoundButton = new ElaPushButton(QStringLiteral("删一轮"), row);
        auto *rewindButton = new ElaPushButton(QStringLiteral("回滚"), row);
        saveButton->setMinimumWidth(TextWidthWithPadding(saveButton, saveButton->text(), 74, 30));
        deleteButton->setMinimumWidth(TextWidthWithPadding(deleteButton, deleteButton->text(), 74, 30));
        deleteRoundButton->setMinimumWidth(
            TextWidthWithPadding(deleteRoundButton, deleteRoundButton->text(), 92, 30));
        rewindButton->setMinimumWidth(TextWidthWithPadding(rewindButton, rewindButton->text(), 74, 30));

        rowLayout->addWidget(indexLabel);
        rowLayout->addWidget(speakerLabel);
        rowLayout->addWidget(edit, 1);
        rowLayout->addWidget(saveButton);
        rowLayout->addWidget(deleteButton);
        rowLayout->addWidget(deleteRoundButton);
        rowLayout->addWidget(rewindButton);

        connect(saveButton, &QPushButton::clicked, this, [this, i, edit]() {
            QStringList historyLines = ReadHistoryLines();
            if (i < 0 || i >= historyLines.size())
                return;
            historyLines[i] = edit->toPlainText();
            SaveHistoryLines(historyLines);
            RefreshHistoryPage();
        });

        connect(deleteButton, &QPushButton::clicked, this, [this, i]() {
            QStringList historyLines = ReadHistoryLines();
            if (i < 0 || i >= historyLines.size())
                return;
            historyLines.removeAt(i);
            SaveHistoryLines(historyLines);
            RefreshHistoryPage();
        });

        connect(deleteRoundButton, &QPushButton::clicked, this, [this, i]() {
            QStringList historyLines = ReadHistoryLines();
            if (i < 0 || i >= historyLines.size())
                return;

            int start = i;
            if (historyLines.at(i).startsWith(QStringLiteral("角色：")) &&
                i > 0 && historyLines.at(i - 1).startsWith(QStringLiteral("用户：")))
                start = i - 1;

            int count = 1;
            if (start + 1 < historyLines.size() &&
                historyLines.at(start).startsWith(QStringLiteral("用户：")) &&
                historyLines.at(start + 1).startsWith(QStringLiteral("角色：")))
                count = 2;

            for (int n = 0; n < count; ++n)
                historyLines.removeAt(start);
            SaveHistoryLines(historyLines);
            RefreshHistoryPage();
        });

        connect(rewindButton, &QPushButton::clicked, this, [this, i]() {
            QStringList historyLines = ReadHistoryLines();
            if (i < 0 || i >= historyLines.size())
                return;
            SaveHistoryLines(historyLines.mid(0, i + 1));
            RefreshHistoryPage();
        });

        m_historyListLayout->insertWidget(insertIndex + m_historyRows.size(), row);
        m_historyRows.append(row);
    }
}

/*加载角色配置*/
void SettingChild_Char::LoadCurrentCharConfig()
{
    QString charName = ui->comboBox_CharList->currentText();
    if (charName.isEmpty() || charName == "未选择")
    {
        //如果没有选择角色，清空配置项显示
        ui->plainTextEdit_CharPrompt->clear();
        if (m_petPromptEdit)
            m_petPromptEdit->clear();
        ui->spinBox_TachieSize->setValue(0);
        ClearTachieBindingRows();
        ui->comboBox_ModelSelect->clear();
        ui->ToggleSwitch_VitsEnable->setIsToggled(false);
        ui->comboBox_Vits_MASSelect->clear();
        ui->comboBox_Vits_MASSelect->setEnabled(false);
        ui->comboBox_Vits_ServerSelect->setEnabled(false);
        return;
    }

    //角色提示词
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    QString charPrompt = charConfig.value("prompt").toString();
    ui->plainTextEdit_CharPrompt->setPlainText(charPrompt);
    if (m_petPromptEdit)
    {
        QString petPrompt = charConfig.value("petPrompt").toString();
        if (petPrompt.isEmpty())
        {
            petPrompt = DefaultPetPrompt();
            charConfig.setValue("petPrompt", petPrompt);
        }
        m_petPromptEdit->setPlainText(petPrompt);
    }
    //立绘大小
    ZcJsonLib charUserConfig(CharacterUserConfigPath + "/" + charName +
                             "/config.json");
    QString tachieSize = charUserConfig.value("tachieSize").toString();
    ui->spinBox_TachieSize->setValue(tachieSize.toInt());
    //立绘动作和动画绑定
    RefreshTachieActionList();
    //服务商和模型选择
    QString serverSelect = charUserConfig.value("serverSelect").toString();
    if (serverSelect.isEmpty())
        serverSelect = "DeepSeek";
    ui->comboBox_ServerSelect->setCurrentText(serverSelect);
    RefreshModelList();
    //模型选择
    QString modelSelect = charUserConfig.value("modelSelect").toString();
    if (modelSelect.isEmpty())
        modelSelect = ui->comboBox_ModelSelect->currentText();
    if (!serverSelect.isEmpty())
        charUserConfig.setValue("serverSelect", serverSelect);
    if (!modelSelect.isEmpty())
        charUserConfig.setValue("modelSelect", modelSelect);
    ui->comboBox_ModelSelect->setCurrentText(modelSelect);
    //Vits语音合成选择
    bool vitsEnable = charUserConfig.value("vitsEnable").toBool();
    ui->ToggleSwitch_VitsEnable->setIsToggled(vitsEnable);
    ui->comboBox_Vits_MASSelect->setEnabled(vitsEnable);
    ui->comboBox_Vits_ServerSelect->setEnabled(vitsEnable);
    //Vits模型选择
    ZcJsonLib config(JsonSettingPath);
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    QStringList vitsMasList;
    for (const QJsonValue &val : arr)
        vitsMasList.append(val.toString());
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
    //读取当前选择的Vits模型
    QString vitsMasSelect = charUserConfig.value("vitsMasSelect").toString();
    ui->comboBox_Vits_MASSelect->setCurrentText(vitsMasSelect);
}

/*刷新角色列表*/
void SettingChild_Char::RefreshCharList()
{
    ui->comboBox_CharList->clear();
    ui->comboBox_CharList->addItem(PrimaryCharacterName());
}

/*修改选中角色*/
void SettingChild_Char::on_comboBox_CharList_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    if (arg1 != PrimaryCharacterName())
        return;
    //保存到配置文件
    QSettings *settings =
        new QSettings(IniSettingPath, QSettings::IniFormat, this);
    settings->setValue("character/CharSelect", arg1);

    isAlreadyLoading = false;
    LoadCurrentCharConfig();
    isAlreadyLoading = true;

    emit requestReloadCharSelect("default");
    emit requestReloadAIConfig();
}

/*修改角色提示词*/
void SettingChild_Char::on_plainTextEdit_CharPrompt_textChanged()
{
    if (!isAlreadyLoading)
        return;
    //保存到角色文件夹下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString charPrompt = ui->plainTextEdit_CharPrompt->toPlainText();
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    charConfig.setValue("prompt", charPrompt);
}

/*修改立绘大小*/
void SettingChild_Char::on_spinBox_TachieSize_textChanged(const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    //保存到角色配置位置下的config.json
    QString tachieSize = ui->spinBox_TachieSize->text();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("tachieSize", tachieSize);
    emit requestSetTachieSize(arg1.toInt());
}

/*切换服务商选择*/
void SettingChild_Char::on_comboBox_ServerSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("serverSelect", serverSelect);
    RefreshModelList();
    emit requestReloadAIConfig();
}

/*刷新模型列表*/
void SettingChild_Char::RefreshModelList()
{
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    if (serverSelect.isEmpty())
        serverSelect = "DeepSeek";
    const QStringList modelList = ReadLlmModelList(serverSelect);
    ui->comboBox_ModelSelect->clear();
    ui->comboBox_ModelSelect->addItems(modelList);
}

/*刷新Vits模型列表*/
void SettingChild_Char::RefreshVitsModelList()
{
    ZcJsonLib config(JsonSettingPath);
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    QStringList vitsMasList;
    for (const QJsonValue &val : arr)
        vitsMasList.append(val.toString());
    ui->comboBox_Vits_MASSelect->clear();
    ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
}

/*切换模型选择*/
void SettingChild_Char::on_comboBox_ModelSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString modelSelect = ui->comboBox_ModelSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("modelSelect", modelSelect);
    emit requestReloadAIConfig();
}

/*重置立绘位置*/
void SettingChild_Char::on_pushButton_ResetTachieLoc_clicked()
{
    emit requestResetTachieLoc();
}

/*切换语音合成模型选择*/
void SettingChild_Char::on_comboBox_Vits_MASSelect_currentTextChanged(
    const QString &arg1)
{
    if (!isAlreadyLoading)
        return;
    Q_UNUSED(arg1)
    //保存到角色配置位置下的config.json
    QString vitsMasSelect = ui->comboBox_Vits_MASSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("vitsMasSelect", vitsMasSelect);
}

/*切换是否启用Vits*/
void SettingChild_Char::on_ToggleSwitch_VitsEnable_toggled(bool checked)
{
    if (!isAlreadyLoading)
        return;
    //保存到角色配置位置下的config.json
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("vitsEnable", checked);
    ui->comboBox_Vits_MASSelect->setEnabled(checked);
    ui->comboBox_Vits_ServerSelect->setEnabled(checked);
}

/*刷新立绘动画列表*/
void SettingChild_Char::RefreshTachieAnimationList()
{
    //兼容旧调用：现在动画绑定随动作行一起刷新
    RefreshTachieActionList();
}

/*清理动态创建的动作绑定行*/
void SettingChild_Char::ClearTachieBindingRows()
{
    for (QWidget *row : m_tachieBindingRows)
    {
        if (row)
            row->deleteLater();
    }
    m_tachieBindingRows.clear();
}

/*刷新立绘动作列表*/
void SettingChild_Char::RefreshTachieActionList()
{
    QString charName = ui->comboBox_CharList->currentText();
    ClearTachieBindingRows();
    if (charName.isEmpty() || charName == "未选择")
        return;

    //扫描角色立绘文件夹，获取所有动作名称
    QDir tachieDir(CharacterAssestPath + "/" + charName + "/Tachie");
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg";
    QStringList fileNames = tachieDir.entryList(nameFilters, QDir::Files);

    QStringList actionList;
    for (const QString &fileName : fileNames)
    {
        QString actionName = QFileInfo(fileName).completeBaseName();
        if (!actionName.isEmpty() && !actionList.contains(actionName))
            actionList.append(actionName);
    }
    if (actionList.isEmpty())
        actionList.append("default");

    //动画候选列表：统一使用唯一键（插件名_动画名）
    const QStringList animationUniqueKeys = m_pluginManager.AnimationUniqueKeys();

    ZcJsonLib charUserConfig(CharacterAssestPath + "/" + charName +
                             "/config.json");
    QJsonObject animationMap =
        charUserConfig.value("tachieAnimations", QJsonObject()).toObject();

    //创建横排行：左侧动作名称，右侧绑定下拉
    QVBoxLayout *layout = ui->verticalLayout_TachieBindingList;
    const int insertIndex = qMax(0, layout->count() - 1); //在底部弹簧前插入

    for (const QString &actionName : actionList)
    {
        ElaScrollPageArea *row =
            new ElaScrollPageArea(ui->widget_TachieBindingContainer);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(15, 0, 15, 0);

        ElaText *label = new ElaText(row);
        QFont font = label->font();
        font.setPointSize(12);
        label->setFont(font);
        label->setText(actionName);

        ElaComboBox *combo = new ElaComboBox(row);
        combo->setStyleSheet(ui->comboBox_CharList->styleSheet());
        combo->addItem("无动画", "");
        for (const QString &uniqueKey : animationUniqueKeys)
        {
            combo->addItem(uniqueKey, uniqueKey);
        }

        QString boundAnimation = animationMap.value(actionName).toString();

        combo->blockSignals(true);
        const int keyIndex = combo->findData(boundAnimation);
        if (keyIndex >= 0)
            combo->setCurrentIndex(keyIndex);
        else
            combo->setCurrentIndex(0);
        combo->blockSignals(false);

        QObject::connect(combo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, actionName, combo](int index)
                         {
                             Q_UNUSED(index)
                             if (!isAlreadyLoading)
                                 return;

                             QString currentCharName =
                                 ui->comboBox_CharList->currentText();
                             ZcJsonLib charConfig(CharacterAssestPath + "/" +
                                                  currentCharName +
                                                  "/config.json");
                             QJsonObject map =
                                 charConfig.value("tachieAnimations", QJsonObject())
                                     .toObject();
                             const QString selectedUniqueKey =
                                 combo->currentData().toString();
                             if (selectedUniqueKey.isEmpty())
                                 map.remove(actionName);
                             else
                                 map.insert(actionName, selectedUniqueKey);

                             charConfig.setValue("tachieAnimations", map);
                         });

        rowLayout->addWidget(label, 3);
        rowLayout->addWidget(combo, 2);
        layout->insertWidget(insertIndex + m_tachieBindingRows.size(), row);
        m_tachieBindingRows.append(row);
    }
}

//进入立绘设置二级菜单
void SettingChild_Char::on_pushButton_Tachie_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("立绘设置");
}

//面包屑导航：返回上级
void SettingChild_Char::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(lastBreadcrumbList)
    if (breadcrumb == "角色设置")
    {
        ShowPrimaryPage();
    }
}
