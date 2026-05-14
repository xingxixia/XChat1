#ifndef SETTINGCHILD_TEXT_H
#define SETTINGCHILD_TEXT_H

#include <QWidget>

class QComboBox;
class QSpinBox;

class SettingChild_Text : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_Text(QWidget *parent = nullptr);

  signals:
    void textSettingsChanged();

  private:
    void loadConfig();
    void saveConfig();

    QComboBox *m_fontPresetComboBox = nullptr;
    QComboBox *m_textBoxThemeComboBox = nullptr;
    QSpinBox *m_fontSizeSpinBox = nullptr;
    bool m_loading = false;
};

#endif // SETTINGCHILD_TEXT_H
