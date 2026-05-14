#ifndef DIALOG_H
#define DIALOG_H

#include "AiProvider.h"
#include <QEvent>
#include <QList>
#include <QMoveEvent>
#include <QStringList>
#include <QWidget>

class QAudioOutput;
class QMediaPlayer;
class QNetworkAccessManager;
class QTemporaryFile;
class QTextEdit;

namespace Ui
{
class Dialog;
}

class history;

class Dialog : public QWidget
{
    Q_OBJECT

  public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

  public slots:
    void ToggleVisible();
    void VitsGetAndPlay(QString text);
    void ReloadAIConfig();
    void ReloadTextSettings();

  private slots:
    void on_pushButton_next_clicked();
    void on_pushButton_history_clicked();
    void rewindToHistoryIndex(int historyIndex);

  signals:
    void requestSetCharTachie(QString TachieName);

  private:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void initWindow();
    void handleWheelUp();
    void handleWheelDown();

    void loadContextHistory();
    void saveContextHistory() const;
    void appendHistoryLine(const QString &line);
    QString buildUserMessageWithContext(const QString &input) const;
    void rewindDisplayedHistoryLine(const QString &line);

    bool isAwaitingNextInput() const;
    void prepareUserInput();
    void showReplyText(const QString &text);
    void showInputText(const QString &text = QString());
    void submitCurrentInput();
    void stopPendingConversationState();

    void tryStartNextVitsRequest();
    void tryStartNextVitsPlayback();

    Ui::Dialog *ui = nullptr;
    history *historyWin = nullptr;
    QTextEdit *m_replyTextEdit = nullptr;
    AiProvider *ai = nullptr;

    QPoint lastPos;
    QList<int> keys;
    bool isHistoryOpen = false;

    QStringList m_contextHistory;
    QString m_lastUserInput;
    QString m_streamRawReply;
    QString m_streamDisplayedChinese;
    bool m_streamVitsEnabled = false;
    bool m_streamVitsSentenceSplitEnabled = true;
    int m_streamSynthCursor = 0;
    QStringList m_vitsPendingTexts;
    QList<QTemporaryFile *> m_vitsReadyFiles;
    bool m_vitsRequestInFlight = false;
    QNetworkAccessManager *m_vitsManager = nullptr;
    QMediaPlayer *m_vitsPlayer = nullptr;
    QAudioOutput *m_vitsAudioOutput = nullptr;
    QTemporaryFile *m_vitsTempFile = nullptr;
    QString m_textBoxTheme = QStringLiteral("light");
};

#endif // DIALOG_H
