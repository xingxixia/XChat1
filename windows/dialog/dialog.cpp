#include "dialog.h"
#include "history/history.h"
#include "ui_dialog.h"

#include "../../GlobalConstants.h"
#include "../../utils/CustomScrollBinder.h"
#include "../../utils/DragHelper.h"
#include "ZcJsonLib.h"

#include <QAudioOutput>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QJsonArray>
#include <QKeyEvent>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QShowEvent>
#include <QTemporaryFile>
#include <QTextCursor>
#include <QTextEdit>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>

namespace
{
constexpr const char *kUserPrefix = "用户：";
constexpr const char *kRolePrefix = "角色：";

int findNextSentenceEnd(const QString &text, int start)
{
    for (int i = qMax(0, start); i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if (ch == QChar('.') || ch == QChar('!') || ch == QChar('?') ||
            ch == QChar('\n') || ch == QChar(';') || ch == QChar(u'。') ||
            ch == QChar(u'！') || ch == QChar(u'？') || ch == QChar(u'；'))
            return i;
    }
    return -1;
}
}

void Dialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QRectF rect(5, 5, width() - 10, height() - 10);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool oldDarkTheme = (m_textBoxTheme == QStringLiteral("dark"));
    const bool galLightTheme = (m_textBoxTheme == QStringLiteral("gal_light"));
    const bool galDarkTheme = (m_textBoxTheme == QStringLiteral("gal_dark"));
    const bool galTheme = galLightTheme || galDarkTheme;

    QColor shadowColor = (oldDarkTheme || galDarkTheme) ? QColor(0, 0, 0, 95)
                                                        : QColor(0, 0, 0, 48);
    for (int i = 0; i < 5; ++i)
    {
        QPainterPath shadowPath;
        shadowPath.setFillRule(Qt::WindingFill);
        QRectF shadowRect((5 - i), (5 - i), width() - (5 - i) * 2,
                          height() - (5 - i) * 2);
        shadowPath.addRoundedRect(shadowRect, 15, 15);
        shadowColor.setAlpha(((oldDarkTheme || galDarkTheme) ? 95 : 48) - qSqrt(i) * 20);
        painter.setPen(shadowColor);
        painter.drawPath(shadowPath);
    }

    if (galTheme)
    {
        const QRectF bodyRect = rect.adjusted(4, 32, -4, -4);
        const bool dark = galDarkTheme;

        QPainterPath bodyPath;
        bodyPath.addRoundedRect(bodyRect, 20, 20);

        QLinearGradient bodyGradient(bodyRect.topLeft(), bodyRect.bottomLeft());
        if (dark)
        {
            bodyGradient.setColorAt(0.0, QColor(35, 39, 58, 230));
            bodyGradient.setColorAt(0.55, QColor(23, 27, 42, 218));
            bodyGradient.setColorAt(1.0, QColor(15, 18, 30, 208));
        }
        else
        {
            bodyGradient.setColorAt(0.0, QColor(255, 252, 248, 232));
            bodyGradient.setColorAt(0.52, QColor(255, 244, 232, 220));
            bodyGradient.setColorAt(1.0, QColor(244, 228, 214, 210));
        }
        painter.fillPath(bodyPath, bodyGradient);

        painter.setPen(QPen(dark ? QColor(168, 194, 255, 145)
                                 : QColor(255, 255, 255, 230),
                            1.5));
        painter.drawPath(bodyPath);

        painter.setPen(QPen(dark ? QColor(255, 255, 255, 40)
                                 : QColor(156, 104, 72, 46),
                            1.0));
        painter.drawRoundedRect(bodyRect.adjusted(13, 13, -13, -13), 14, 14);

        QLinearGradient glossGradient(bodyRect.topLeft(), bodyRect.bottomLeft());
        glossGradient.setColorAt(0.0, dark ? QColor(255, 255, 255, 35)
                                           : QColor(255, 255, 255, 115));
        glossGradient.setColorAt(0.38, QColor(255, 255, 255, 12));
        glossGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
        QPainterPath glossPath;
        glossPath.addRoundedRect(bodyRect.adjusted(8, 7, -8, -bodyRect.height() * 0.54), 15, 15);
        painter.fillPath(glossPath, glossGradient);

        const QColor accentColor = dark ? QColor(132, 177, 255, 165)
                                        : QColor(255, 166, 112, 170);
        const QColor cornerColor = dark ? QColor(220, 232, 255, 125)
                                        : QColor(255, 255, 255, 180);

        painter.setPen(QPen(accentColor, 2.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(bodyRect.right() - 150, bodyRect.top() + 18),
                         QPointF(bodyRect.right() - 34, bodyRect.top() + 18));
        painter.setPen(QPen(accentColor, 1.6, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(bodyRect.right() - 118, bodyRect.bottom() - 18),
                         QPointF(bodyRect.right() - 28, bodyRect.bottom() - 18));

        painter.setPen(QPen(cornerColor, 1.5, Qt::SolidLine, Qt::RoundCap));
        const qreal corner = 18.0;
        painter.drawLine(bodyRect.topLeft() + QPointF(18, 8), bodyRect.topLeft() + QPointF(18 + corner, 8));
        painter.drawLine(bodyRect.topLeft() + QPointF(8, 18), bodyRect.topLeft() + QPointF(8, 18 + corner));
        painter.drawLine(bodyRect.topRight() + QPointF(-18, 8), bodyRect.topRight() + QPointF(-18 - corner, 8));
        painter.drawLine(bodyRect.topRight() + QPointF(-8, 18), bodyRect.topRight() + QPointF(-8, 18 + corner));
        painter.drawLine(bodyRect.bottomLeft() + QPointF(18, -8), bodyRect.bottomLeft() + QPointF(18 + corner, -8));
        painter.drawLine(bodyRect.bottomLeft() + QPointF(8, -18), bodyRect.bottomLeft() + QPointF(8, -18 - corner));
        painter.drawLine(bodyRect.bottomRight() + QPointF(-18, -8), bodyRect.bottomRight() + QPointF(-18 - corner, -8));
        painter.drawLine(bodyRect.bottomRight() + QPointF(-8, -18), bodyRect.bottomRight() + QPointF(-8, -18 - corner));
        return;
    }

    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRoundedRect(rect, 18, 18);

    const QColor fillColor = oldDarkTheme ? QColor(28, 30, 38, 218)
                                          : QColor(255, 250, 242, 218);
    const QColor borderColor = oldDarkTheme ? QColor(230, 238, 255, 105)
                                            : QColor(255, 255, 255, 190);
    const QColor accentColor = oldDarkTheme ? QColor(132, 178, 255, 120)
                                            : QColor(255, 198, 130, 135);

    painter.fillPath(path, QBrush(fillColor));
    painter.setPen(QPen(borderColor, 1.4));
    painter.drawPath(path);

    QRectF accentRect = rect.adjusted(18, 12, -18, -rect.height() + 16);
    QPainterPath accentPath;
    accentPath.addRoundedRect(accentRect, 6, 6);
    painter.fillPath(accentPath, accentColor);

    QRectF innerLine = rect.adjusted(16, 17, -16, -17);
    painter.setPen(QPen(oldDarkTheme ? QColor(255, 255, 255, 38)
                                     : QColor(255, 255, 255, 150),
                        1.0));
    painter.drawRoundedRect(innerLine, 13, 13);
}

Dialog::Dialog(QWidget *parent) : QWidget(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);
    initWindow();

    ai = new AiProvider(this);
    ai->setStreamEnabled(true);

    m_vitsManager = new QNetworkAccessManager(this);
    m_vitsPlayer = new QMediaPlayer(this);
    m_vitsAudioOutput = new QAudioOutput(this);
    m_vitsPlayer->setAudioOutput(m_vitsAudioOutput);
    connect(m_vitsPlayer, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state)
            {
                if (state == QMediaPlayer::StoppedState)
                {
                    if (m_vitsTempFile)
                    {
                        m_vitsTempFile->deleteLater();
                        m_vitsTempFile = nullptr;
                    }
                    tryStartNextVitsPlayback();
                }
            });

    ReloadAIConfig();

    connect(ai, &AiProvider::replyChunkReceived, this,
            [this](const QString &chunk)
            {
                m_streamRawReply += chunk;
                const int firstSep = m_streamRawReply.indexOf('|');
                if (firstSep < 0)
                    return;

                const int secondSep = m_streamRawReply.indexOf('|', firstSep + 1);
                const int chineseEnd = secondSep < 0 ? m_streamRawReply.size() : secondSep;
                const QString chinesePartial = m_streamRawReply.mid(firstSep + 1, chineseEnd - firstSep - 1);
                if (!chinesePartial.isEmpty() && chinesePartial != m_streamDisplayedChinese)
                {
                    m_streamDisplayedChinese = chinesePartial;
                    showReplyText(m_streamDisplayedChinese);
                }

                if (m_streamVitsEnabled && m_streamVitsSentenceSplitEnabled && secondSep >= 0)
                {
                    const QString japanesePartial = m_streamRawReply.mid(secondSep + 1);
                    int sentenceEnd = findNextSentenceEnd(japanesePartial, m_streamSynthCursor);
                    while (sentenceEnd >= 0)
                    {
                        const QString sentence = japanesePartial.mid(m_streamSynthCursor,
                                                                     sentenceEnd - m_streamSynthCursor + 1).trimmed();
                        m_streamSynthCursor = sentenceEnd + 1;
                        if (!sentence.isEmpty())
                            VitsGetAndPlay(sentence);
                        sentenceEnd = findNextSentenceEnd(japanesePartial, m_streamSynthCursor);
                    }
                }
            });

    connect(ai, &AiProvider::replyReceived, this,
            [this](const QString &reply)
            {
                const QString finalReply = m_streamRawReply.isEmpty() ? reply : m_streamRawReply;
                const QString mood = finalReply.section('|', 0, 0).trimmed();
                const QString chineseReply = finalReply.section('|', 1, 1).trimmed();
                const QString japaneseReply = finalReply.section('|', 2, 2).trimmed();
                const QString displayedReply = chineseReply.isEmpty() ? finalReply.trimmed() : chineseReply;

                ui->pushButton_next->show();
                showReplyText(displayedReply);

                if (m_streamVitsEnabled)
                {
                    if (m_streamVitsSentenceSplitEnabled)
                    {
                        const QString remainJapanese = japaneseReply.mid(qMax(0, m_streamSynthCursor)).trimmed();
                        if (!remainJapanese.isEmpty())
                            VitsGetAndPlay(remainJapanese);
                    }
                    else if (!japaneseReply.isEmpty())
                    {
                        VitsGetAndPlay(japaneseReply);
                    }
                }

                emit requestSetCharTachie(mood);
                if (!m_lastUserInput.isEmpty())
                {
                    appendHistoryLine(QString::fromUtf8(kUserPrefix) + m_lastUserInput);
                    m_lastUserInput.clear();
                }
                appendHistoryLine(QString::fromUtf8(kRolePrefix) + displayedReply);
                saveContextHistory();

                m_streamRawReply.clear();
                m_streamDisplayedChinese.clear();
                m_streamVitsEnabled = false;
                m_streamSynthCursor = 0;
            });

    connect(ai, &AiProvider::errorOccurred, this,
            [this](const QString &error)
            {
                ui->pushButton_next->show();
                showReplyText(error);
                m_lastUserInput.clear();
                m_streamRawReply.clear();
                m_streamDisplayedChinese.clear();
                m_streamVitsEnabled = false;
                m_streamSynthCursor = 0;
            });
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::initWindow()
{
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint;
#ifdef Q_OS_LINUX
    flags |= Qt::X11BypassWindowManagerHint;
#endif
    setWindowFlags(flags);
    setWindowOpacity(0.95);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->gridLayout->setContentsMargins(32, 44, 22, 16);
    ui->gridLayout->setVerticalSpacing(0);
    ui->pushButton_next->hide();
    ui->verticalScrollBar->hide();
    ui->label_name->setText(QStringLiteral("你"));
    ui->label_name->setVisible(true);
    ui->label_name->setMinimumHeight(24);

    ui->textEdit->setEnabled(true);
    ui->textEdit->setReadOnly(false);
    ui->textEdit->setPlaceholderText(QStringLiteral("说点什么吧（Shift+Enter 换行，Enter 发送）"));

    m_replyTextEdit = new QTextEdit(this);
    m_replyTextEdit->setObjectName(QStringLiteral("replyTextEdit"));
    m_replyTextEdit->setFrameShape(QFrame::NoFrame);
    m_replyTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_replyTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_replyTextEdit->setAcceptRichText(true);
    m_replyTextEdit->setReadOnly(true);
    m_replyTextEdit->hide();
    ui->gridLayout->addWidget(m_replyTextEdit, 1, 0);

    ReloadTextSettings();
    new CustomScrollBinder(m_replyTextEdit, ui->verticalScrollBar, 5, this);
    new DragHelper(this);

    ui->textEdit->installEventFilter(this);
    ui->textEdit->viewport()->installEventFilter(this);
    m_replyTextEdit->installEventFilter(this);
    m_replyTextEdit->viewport()->installEventFilter(this);
}

void Dialog::keyPressEvent(QKeyEvent *event)
{
    if (!keys.contains(event->key()))
        keys.append(event->key());
    QWidget::keyPressEvent(event);
}

void Dialog::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        keys.removeAll(event->key());
        if (historyWin && historyWin->isVisible())
        {
            historyWin->hide();
            isHistoryOpen = false;
        }
        hide();
        return;
    }

    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        !keys.contains(Qt::Key_Shift))
    {
        if (isAwaitingNextInput())
            prepareUserInput();
        else
            submitCurrentInput();
        keys.removeAll(event->key());
        return;
    }

    keys.removeAll(event->key());
    QWidget::keyReleaseEvent(event);
}

void Dialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ui->label_name->setVisible(true);
    if (!isAwaitingNextInput())
        ui->textEdit->setFocus();
}

void Dialog::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        handleWheelUp();
    else if (event->angleDelta().y() < 0)
        handleWheelDown();
    QWidget::wheelEvent(event);
}

void Dialog::moveEvent(QMoveEvent *event)
{
    if (historyWin && historyWin->isVisible())
    {
        QPoint offset = event->pos() - lastPos;
        historyWin->move(historyWin->pos() + offset);
    }
    lastPos = event->pos();
    QWidget::moveEvent(event);
}

bool Dialog::eventFilter(QObject *watched, QEvent *event)
{
    const bool inputWatched = (watched == ui->textEdit || watched == ui->textEdit->viewport());
    const bool replyWatched = (m_replyTextEdit && (watched == m_replyTextEdit || watched == m_replyTextEdit->viewport()));

    if ((inputWatched || replyWatched) && event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        const bool enterKey = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        if (enterKey && !(keyEvent->modifiers() & Qt::ShiftModifier))
            return true;
    }

    if ((inputWatched || replyWatched) && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        const bool enterKey = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        if (enterKey && !(keyEvent->modifiers() & Qt::ShiftModifier))
        {
            if (isAwaitingNextInput())
                prepareUserInput();
            else
                submitCurrentInput();
            return true;
        }

        if (isAwaitingNextInput() &&
            !keyEvent->text().isEmpty() &&
            !(keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
        {
            const QString typedText = keyEvent->text();
            prepareUserInput();
            if (!typedText.isEmpty() && typedText.at(0).isPrint())
                ui->textEdit->insertPlainText(typedText);
            return true;
        }
    }

    if ((inputWatched || replyWatched) && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        if (wheelEvent->angleDelta().y() > 0)
            handleWheelUp();
        else if (wheelEvent->angleDelta().y() < 0)
            handleWheelDown();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void Dialog::handleWheelUp()
{
    if (!isHistoryOpen)
        ui->pushButton_history->click();
}

void Dialog::handleWheelDown()
{
    if (isHistoryOpen)
        ui->pushButton_history->click();
}

void Dialog::loadContextHistory()
{
    m_contextHistory.clear();
    const QString contextPath = ReadCharacterContextPath();
    if (contextPath.isEmpty())
        return;

    ZcJsonLib contextConfig(contextPath);
    const QJsonArray historyArray = contextConfig.value("history", QJsonValue(QJsonArray())).toArray();
    for (const QJsonValue &value : historyArray)
    {
        const QString line = value.toString();
        if (!line.isEmpty())
            m_contextHistory.append(line);
    }
}

void Dialog::saveContextHistory() const
{
    const QString contextPath = ReadCharacterContextPath();
    if (contextPath.isEmpty())
        return;

    const QFileInfo fileInfo(contextPath);
    QDir().mkpath(fileInfo.absolutePath());

    QJsonArray historyArray;
    for (const QString &line : m_contextHistory)
        historyArray.append(line);

    ZcJsonLib contextConfig(contextPath);
    contextConfig.setValue("history", QJsonValue(historyArray));
}

void Dialog::appendHistoryLine(const QString &line)
{
    if (!line.isEmpty())
        m_contextHistory.append(line);
}

QString Dialog::buildUserMessageWithContext(const QString &input) const
{
    if (m_contextHistory.isEmpty())
        return input;

    return QStringLiteral("以下是最近的对话历史，请结合上下文自然回复：\n") +
           m_contextHistory.join(QStringLiteral("\n")) +
           QStringLiteral("\n\n用户当前输入：") + input;
}

void Dialog::stopPendingConversationState()
{
    m_lastUserInput.clear();
    m_streamRawReply.clear();
    m_streamDisplayedChinese.clear();
    m_streamVitsEnabled = false;
    m_streamSynthCursor = 0;
    m_vitsPendingTexts.clear();
    m_vitsRequestInFlight = false;

    for (QTemporaryFile *file : m_vitsReadyFiles)
    {
        if (file)
            file->deleteLater();
    }
    m_vitsReadyFiles.clear();

    if (m_vitsTempFile)
    {
        m_vitsTempFile->deleteLater();
        m_vitsTempFile = nullptr;
    }
    if (m_vitsPlayer)
        m_vitsPlayer->stop();
}

bool Dialog::isAwaitingNextInput() const
{
    return ui->pushButton_next->isVisible();
}

void Dialog::prepareUserInput()
{
    ui->label_name->setText(QStringLiteral("你"));
    showInputText();
    ui->pushButton_next->hide();
}

void Dialog::showReplyText(const QString &text)
{
    if (!m_replyTextEdit)
        return;
    ui->textEdit->hide();
    m_replyTextEdit->show();
    m_replyTextEdit->setText(text);
    m_replyTextEdit->moveCursor(QTextCursor::Start);
    m_replyTextEdit->setFocus();
}

void Dialog::showInputText(const QString &text)
{
    if (m_replyTextEdit)
        m_replyTextEdit->hide();
    ui->textEdit->show();
    ui->textEdit->setEnabled(true);
    ui->textEdit->setReadOnly(false);
    ui->textEdit->setText(text);
    ui->textEdit->setFocus();
    ui->textEdit->moveCursor(QTextCursor::End);
}

void Dialog::submitCurrentInput()
{
    const QString userInput = ui->textEdit->toPlainText().trimmed();
    if (userInput.isEmpty())
    {
        prepareUserInput();
        return;
    }

    ui->label_name->setText(QStringLiteral("她"));
    ui->pushButton_next->hide();
    ui->textEdit->setReadOnly(true);

    QDir dir(ReadCharacterTachiePath());
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg";
    QStringList fileNames = dir.entryList(nameFilters, QDir::Files);
    QString tachieNameList;
    for (const QString &fileName : fileNames)
        tachieNameList += fileName.section('.', 0, 0) + ", ";

    ZcJsonLib roleConfig(CharacterAssestPath + "/" + ReadNowSelectChar() + "/config.json");
    const QString characterPrompt = roleConfig.value("prompt").toString().trimmed();

    QString systemPrompt;
    if (!characterPrompt.isEmpty())
        systemPrompt += QStringLiteral("角色设定：") + characterPrompt +
                        QStringLiteral("\n请始终保持该设定进行回复。\n\n");
    systemPrompt += QStringLiteral(
        "你是一个桌面陪伴 AI，输出内容必须严格使用以下格式：\n"
        "表情|中文回复|日语配音文本\n\n"
        "要求：\n"
        "1. 表情必须从以下立绘名称中选择：") +
                    tachieNameList + QStringLiteral(
        "\n2. 中文回复是展示给用户看的内容。\n"
        "3. 日语配音文本用于语音合成。\n"
        "4. 不要输出格式以外的说明。\n\n"
        "示例：\n"
        "default|你好呀，需要我陪你看代码吗？|こんにちは、コードを一緒に見ましょうか。\n");
    ai->setSystemPrompt(systemPrompt);

    m_lastUserInput = userInput;
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    m_streamVitsEnabled = charConfig.value("vitsEnable").toBool();
    ZcJsonLib config(JsonSettingPath);
    m_streamVitsSentenceSplitEnabled = config.value("vits/SentenceSplit", true).toBool();

    m_streamRawReply.clear();
    m_streamDisplayedChinese.clear();
    m_streamSynthCursor = 0;
    m_vitsPendingTexts.clear();
    for (QTemporaryFile *file : m_vitsReadyFiles)
    {
        if (file)
            file->deleteLater();
    }
    m_vitsReadyFiles.clear();
    m_vitsRequestInFlight = false;
    if (m_vitsTempFile)
    {
        m_vitsTempFile->deleteLater();
        m_vitsTempFile = nullptr;
    }
    if (m_vitsPlayer)
        m_vitsPlayer->stop();

    showReplyText(QStringLiteral("......"));
    ai->chat(buildUserMessageWithContext(userInput));
}

void Dialog::on_pushButton_next_clicked()
{
    prepareUserInput();
}

void Dialog::ToggleVisible()
{
    setVisible(!isVisible());
}

void Dialog::ReloadAIConfig()
{
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    QString serverSelect = charConfig.value("serverSelect").toString();
    if (serverSelect == "DeepSeek")
        ai->setServiceType(AiProvider::DeepSeek);
    else if (serverSelect == "OpenAI")
        ai->setServiceType(AiProvider::OpenAI);
    else
    {
        serverSelect = "DeepSeek";
        ai->setServiceType(AiProvider::DeepSeek);
    }

    ZcJsonLib config(JsonSettingPath);
    ai->setApiKey(config.value("llm/" + serverSelect + "/ApiKey").toString());
    ai->setModel(charConfig.value("modelSelect").toString());
    loadContextHistory();
}

void Dialog::ReloadTextSettings()
{
    ZcJsonLib config(JsonSettingPath);
    QString fontFamily = config.value("text/fontFamily").toString().trimmed();
    int fontSize = config.value("text/fontSize").toInt();
    m_textBoxTheme = config.value("text/textBoxTheme").toString().trimmed();
    if (m_textBoxTheme != QStringLiteral("dark") &&
        m_textBoxTheme != QStringLiteral("gal_light") &&
        m_textBoxTheme != QStringLiteral("gal_dark"))
        m_textBoxTheme = QStringLiteral("light");

    const bool darkTheme = (m_textBoxTheme == QStringLiteral("dark") ||
                            m_textBoxTheme == QStringLiteral("gal_dark"));
    const QString textColor = darkTheme ? QStringLiteral("#F4F0EA")
                                        : QStringLiteral("#222222");
    const QString nameColor =
        (m_textBoxTheme == QStringLiteral("gal_dark"))
            ? QStringLiteral("#FFF1C6")
            : (darkTheme ? QStringLiteral("#FFE9B6")
                         : QStringLiteral("#3A2B1F"));
    const QString selectionColor = darkTheme ? QStringLiteral("#395D92")
                                             : QStringLiteral("#BBD7FF");
    const QString menuColor = darkTheme ? QStringLiteral("#2C303A")
                                        : QStringLiteral("#FFFFFF");

    ui->label_name->setStyleSheet(
        QStringLiteral("QLabel { color: %1; background: transparent; }").arg(nameColor));

    const QString textEditStyle = QStringLiteral(
        "QTextEdit { color: %1; background: transparent; border: none; selection-background-color: %2; }"
        "QTextEdit:disabled { color: %1; }"
        "QTextEdit:read-only { color: %1; }"
        "QMenu { background-color: %3; color: %1; border: none; border-radius: 8px; padding: 5px; }"
        "QMenu::item { padding: 8px 20px; background-color: transparent; border-radius: 4px; }"
        "QMenu::item:selected { background-color: %2; color: %1; }")
        .arg(textColor, selectionColor, menuColor);
    ui->textEdit->setStyleSheet(textEditStyle);
    if (m_replyTextEdit)
        m_replyTextEdit->setStyleSheet(textEditStyle);

    QFont textFont = ui->textEdit->font();
    if (!fontFamily.isEmpty())
        textFont.setFamily(fontFamily);
    if (fontSize <= 0)
        fontSize = 12;
    textFont.setPointSize(fontSize);
    ui->textEdit->setFont(textFont);
    if (m_replyTextEdit)
        m_replyTextEdit->setFont(textFont);

    QFont nameFont = ui->label_name->font();
    if (!fontFamily.isEmpty())
        nameFont.setFamily(fontFamily);
    nameFont.setPointSize(qMax(10, fontSize));
    ui->label_name->setFont(nameFont);

    update();
}

void Dialog::on_pushButton_history_clicked()
{
    if (!historyWin)
    {
        historyWin = new history(this);
        connect(historyWin, &history::jumpToHistory, this, &Dialog::rewindToHistoryIndex);
    }

    historyWin->clearHistory();
    for (int i = 0; i < m_contextHistory.size(); ++i)
    {
        const QString &line = m_contextHistory.at(i);
        if (line.startsWith(QString::fromUtf8(kUserPrefix)))
            historyWin->addChildWindow(i, QStringLiteral("你"), line.mid(QString::fromUtf8(kUserPrefix).size()));
        else if (line.startsWith(QString::fromUtf8(kRolePrefix)))
            historyWin->addChildWindow(i, QStringLiteral("她"), line.mid(QString::fromUtf8(kRolePrefix).size()));
        else
            historyWin->addChildWindow(i, QStringLiteral("记录"), line);
    }

    historyWin->move(x(), y() - historyWin->height());
    if (!isHistoryOpen)
    {
        historyWin->setWindowOpacity(0);
        historyWin->show();
        QPropertyAnimation *opacityAnim = new QPropertyAnimation(historyWin, "windowOpacity");
        opacityAnim->setDuration(160);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
        isHistoryOpen = true;
    }
    else
    {
        historyWin->hide();
        isHistoryOpen = false;
    }
}

void Dialog::rewindToHistoryIndex(int historyIndex)
{
    if (historyIndex < 0 || historyIndex >= m_contextHistory.size())
        return;

    stopPendingConversationState();
    m_contextHistory = m_contextHistory.mid(0, historyIndex + 1);
    saveContextHistory();

    rewindDisplayedHistoryLine(m_contextHistory.at(historyIndex));
    if (historyWin && isHistoryOpen)
        on_pushButton_history_clicked();
}

void Dialog::rewindDisplayedHistoryLine(const QString &line)
{
    const QString userPrefix = QString::fromUtf8(kUserPrefix);
    const QString rolePrefix = QString::fromUtf8(kRolePrefix);
    if (line.startsWith(userPrefix))
    {
        ui->label_name->setText(QStringLiteral("你"));
        showInputText(line.mid(userPrefix.size()));
        ui->pushButton_next->hide();
    }
    else if (line.startsWith(rolePrefix))
    {
        ui->label_name->setText(QStringLiteral("她"));
        showReplyText(line.mid(rolePrefix.size()));
        ui->pushButton_next->show();
    }
    else
    {
        ui->label_name->setText(QStringLiteral("记录"));
        showReplyText(line);
        ui->pushButton_next->show();
    }
}

void Dialog::VitsGetAndPlay(QString text)
{
    if (text.trimmed().isEmpty())
        return;
    m_vitsPendingTexts.append(text);
    tryStartNextVitsRequest();
}

void Dialog::tryStartNextVitsRequest()
{
    if (!m_vitsManager || !m_vitsPlayer)
        return;
    if (m_vitsRequestInFlight || m_vitsPendingTexts.isEmpty())
        return;

    const QString text = m_vitsPendingTexts.takeFirst();
    if (text.isEmpty())
        return;

    ZcJsonLib config(JsonSettingPath);
    const QString apiUrl = config.value("vits/ApiUrl").toString();
    if (apiUrl.isEmpty())
        return;

    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    const QString modelAndSpeaker = charConfig.value("vitsMasSelect").toString();
    const QString model = modelAndSpeaker.section(" - ", 0, 0).trimmed().toLower();
    const QString speaker = modelAndSpeaker.section(" - ", 2, 2).trimmed();

    QString urlString = QString(apiUrl + "/voice/%2?id=%3&text=%1")
                            .arg(QString(QUrl::toPercentEncoding(text)))
                            .arg(QString(QUrl::toPercentEncoding(model)))
                            .arg(QString(QUrl::toPercentEncoding(speaker)));

    m_vitsRequestInFlight = true;
    QNetworkReply *reply = m_vitsManager->get(QNetworkRequest(QUrl(urlString)));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            {
                m_vitsRequestInFlight = false;
                const QByteArray audioData = reply->readAll();
                reply->deleteLater();

                if (!audioData.isEmpty())
                {
                    QTemporaryFile *readyFile = new QTemporaryFile(QDir::tempPath() + "/vits_XXXXXX.mp3", this);
                    if (readyFile->open())
                    {
                        readyFile->write(audioData);
                        readyFile->flush();
                        readyFile->close();
                        m_vitsReadyFiles.append(readyFile);
                        tryStartNextVitsPlayback();
                    }
                    else
                    {
                        readyFile->deleteLater();
                    }
                }

                tryStartNextVitsRequest();
            });
}

void Dialog::tryStartNextVitsPlayback()
{
    if (!m_vitsPlayer)
        return;
    if (m_vitsPlayer->playbackState() != QMediaPlayer::StoppedState)
        return;
    if (m_vitsReadyFiles.isEmpty())
        return;

    if (m_vitsTempFile)
    {
        m_vitsTempFile->deleteLater();
        m_vitsTempFile = nullptr;
    }

    m_vitsTempFile = m_vitsReadyFiles.takeFirst();
    if (!m_vitsTempFile)
        return;

    m_vitsPlayer->setSource(QUrl::fromLocalFile(m_vitsTempFile->fileName()));
    m_vitsPlayer->play();
}
