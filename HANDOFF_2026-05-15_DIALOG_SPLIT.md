# ZcChat2 对话框拆分对接记录

时间：2026-05-15

## 当前用户目标

用户想把聊天框逻辑从根上拆开：

- 一个控件专门显示 AI/角色回复
- 一个控件专门作为用户输入框
- 两个控件代码位置尽量相邻，方便继续学习和维护

原因是原项目把显示和输入都塞在 `ui->textEdit` 里，导致：

- 角色回复结束后，直接按 Enter 会把角色刚回复的话又当成用户输入发出去
- 回复结束后不点鼠标，直接打字没有正常进入输入态
- 当前状态靠 `textEdit` 是否只读、按钮是否显示等间接判断，容易继续出交互 bug

## 本轮已经做过的修改

涉及文件：

- `windows/dialog/dialog.h`
- `windows/dialog/dialog.cpp`

### 1. `dialog.h` 已新增声明

新增了 `QTextEdit` 前置声明：

```cpp
class QTextEdit;
```

新增了几个私有函数声明：

```cpp
bool isAwaitingNextInput() const;
void prepareUserInput();
void submitCurrentInput();
void showReplyText(const QString &text);
void showInputText(const QString &text = QString());
```

新增了成员变量：

```cpp
QTextEdit *m_replyTextEdit = nullptr;
```

注意：`submitCurrentInput()` 目前只是一个很薄的占位函数，真正旧发送逻辑还在 `keyReleaseEvent()` 里，没有完全搬过去。

### 2. `dialog.cpp` 已开始创建回复显示框

已加入：

```cpp
#include <QTextEdit>
```

在 `Dialog::initWindow()` 附近，已经创建了新的 `m_replyTextEdit`：

```cpp
m_replyTextEdit = new QTextEdit(this);
m_replyTextEdit->setObjectName(QStringLiteral("replyTextEdit"));
m_replyTextEdit->setFrameShape(QFrame::NoFrame);
m_replyTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_replyTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_replyTextEdit->setAcceptRichText(true);
m_replyTextEdit->setReadOnly(true);
m_replyTextEdit->setVisible(false);
ui->gridLayout->addWidget(m_replyTextEdit, 1, 0);
```

并且把滚动条绑定从输入框改到了回复框：

```cpp
new CustomScrollBinder(m_replyTextEdit, ui->verticalScrollBar, 5, this);
```

同时给回复框安装了事件过滤器：

```cpp
m_replyTextEdit->installEventFilter(this);
m_replyTextEdit->viewport()->installEventFilter(this);
```

### 3. 已新增两个切换函数

当前 `dialog.cpp` 中已经有：

```cpp
void Dialog::showReplyText(const QString &text)
{
    if (!m_replyTextEdit)
        return;
    ui->textEdit->hide();
    m_replyTextEdit->show();
    m_replyTextEdit->setText(text);
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
}
```

`prepareUserInput()` 已经改成调用 `showInputText()`：

```cpp
void Dialog::prepareUserInput()
{
    ui->label_name->setText(QStringLiteral("你"));
    showInputText();
    ui->pushButton_next->hide();
}
```

由于原文件编码显示混乱，实际文件里部分中文字符串可能在工具输出里显示成乱码，但代码本意是这个。

### 4. 部分回复写入点已经改成 `showReplyText`

已改的点包括：

- 流式回复分片显示：

```cpp
showReplyText(m_streamDisplayedChinese);
```

- AI 最终回复显示：

```cpp
showReplyText(chineseReply);
```

- API 错误显示：

```cpp
showReplyText(error);
```

## 当前未完成/风险点

### 1. 本轮最后一次补丁失败

我准备继续替换以下残留点时，`apply_patch` 失败：

```text
invalid utf-8 sequence
```

说明 `windows/dialog/dialog.cpp` 里可能存在非 UTF-8 字节或编码混乱。之前文件已经有大量中文注释乱码，这不是本轮才出现的问题，但现在会影响 `apply_patch` 精确修改。

不要贸然整文件转码，容易扩大改动。建议下次用更小范围、或先用 VS/Qt Creator 确认文件编码。

### 2. 残留 `ui->textEdit->setText(...)` 还没全部迁移

需要继续检查：

```powershell
rg -n "ui->textEdit->setText|showReplyText|showInputText|m_replyTextEdit" windows/dialog/dialog.cpp windows/dialog/dialog.h
```

重点残留：

- 发送后显示“……”的地方还可能仍写入 `ui->textEdit`
- 历史回滚 `rewindToHistoryIndex()` 里仍可能用 `ui->textEdit->setText(...)`
- `showInputText()` 自己内部使用 `ui->textEdit->setText(text)` 是正确的，不要替换

目标规则：

- 用户输入内容：只能进入 `ui->textEdit`
- AI/角色回复、错误信息、历史中的角色消息：进入 `m_replyTextEdit`
- 历史中的用户消息如果要恢复成可编辑输入：进入 `ui->textEdit`

### 3. `on_pushButton_next_clicked()` 还需要改干净

当前按钮函数里可能仍是旧逻辑：

```cpp
ui->label_name->setText(...);
ui->textEdit->clear();
ui->textEdit->setEnabled(true);
ui->textEdit->setReadOnly(false);
ui->pushButton_next->hide();
```

建议最终改成：

```cpp
void Dialog::on_pushButton_next_clicked()
{
    prepareUserInput();
}
```

### 4. `ReloadTextSettings()` 需要同步样式和字体到 `m_replyTextEdit`

当前只给 `ui->textEdit` 设置了 stylesheet 和 font。

最终应该：

```cpp
const QString textEditStyle = QStringLiteral(
    "QTextEdit { color: %1; background: transparent; border: none; selection-background-color: %2; }"
    "QTextEdit:disabled { color: %1; }"
    "QTextEdit:read-only { color: %1; }"
    "QMenu { background-color: %3; color: %1; border: none; border-radius: 8px; padding: 5px; }"
    "QMenu::item { padding: 8px 20px; background-color: transparent; border-radius: 4px; }"
    "QMenu::item:selected { background-color: %2; color: %1; }")
    .arg(textColor, selectionColor,
         darkTheme ? QStringLiteral("#2C303A")
                   : QStringLiteral("#FFFFFF"));

ui->textEdit->setStyleSheet(textEditStyle);
if (m_replyTextEdit)
    m_replyTextEdit->setStyleSheet(textEditStyle);
```

字体也要同步：

```cpp
ui->textEdit->setFont(textFont);
if (m_replyTextEdit)
    m_replyTextEdit->setFont(textFont);
```

### 5. `eventFilter()` 要同时照顾两个框

现在的逻辑大意是：

- 在等待下一次输入时，按 Enter：调用 `prepareUserInput()`，不发送
- 在等待下一次输入时，直接打字：调用 `prepareUserInput()`，再把第一个字插入输入框
- 滚轮事件继续用于历史窗口展开/收起

拆分后，事件来源可能是：

- `ui->textEdit`
- `ui->textEdit->viewport()`
- `m_replyTextEdit`
- `m_replyTextEdit->viewport()`

后续继续改时注意判断不要只写死 `ui->textEdit`。

## 建议下一步

1. 先不要继续做 UI 美化。
2. 先让拆分逻辑编译通过。
3. 按下面顺序收尾：

```text
1. 清理残留 ui->textEdit->setText(...)
2. on_pushButton_next_clicked() 改成 prepareUserInput()
3. ReloadTextSettings() 同步样式/字体到 m_replyTextEdit
4. 编译 Release
5. 实测：AI 回复后直接 Enter、直接打字、Shift+Enter 换行、Esc 隐藏
```

构建命令：

```powershell
cd E:\File\AIuseing\xai\ZcChat2
& cmd.exe /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build .\build-release --config Release'
```

## 当前状态一句话

“拆显示框/输入框”已经开始，结构已经加进去，但还没收尾编译；下次接手时先做编译修复和残留迁移，不要先继续扩功能。
