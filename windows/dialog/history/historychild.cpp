#include "historychild.h"
#include "ui_historychild.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include <QFont>

historychild::historychild(int historyIndex, const QString &name, const QString &msg,
                           QWidget *parent)
    : QWidget(parent), ui(new Ui::historychild), m_historyIndex(historyIndex)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
    //显示不抢占焦点
    setAttribute(Qt::WA_ShowWithoutActivating);
    //获取信息
    ui->label_name->setText(name);
    ui->label_msg->setText(msg);
    ui->label_name->setStyleSheet(
        QStringLiteral("QLabel { color: #222222; background: transparent; }"));
    ui->label_msg->setStyleSheet(
        QStringLiteral("QLabel { color: #222222; background: transparent; }"));
    ui->scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollArea QWidget { background: transparent; }"));

    ZcJsonLib config(JsonSettingPath);
    QString fontFamily = config.value("text/fontFamily").toString().trimmed();
    int fontSize = config.value("text/fontSize").toInt();
    if (fontSize <= 0)
        fontSize = 11;

    QFont nameFont = ui->label_name->font();
    QFont msgFont = ui->label_msg->font();
    if (!fontFamily.isEmpty())
    {
        nameFont.setFamily(fontFamily);
        msgFont.setFamily(fontFamily);
    }
    nameFont.setPointSize(qMax(10, fontSize));
    msgFont.setPointSize(qMax(8, fontSize));
    ui->label_name->setFont(nameFont);
    ui->label_msg->setFont(msgFont);

    ui->pushButton_reSpawnVoice->hide();
}

historychild::~historychild()
{
    delete ui;
}

/*请求回溯*/
void historychild::on_pushButton_jump_clicked()
{
    emit jumpRequested(m_historyIndex);
}
