// 项目自己的窗口类：对话框、设置窗口、立绘窗口。
#include "windows/dialog/dialog.h"
#include "windows/setting/setting.h"
#include "windows/tachie/tachie.h"

// ElaWidgetTools 第三方 UI 库：提供应用初始化和更美观的菜单控件。
#include "ElaApplication.h"
#include "ElaMenu.h"

// CMake 根据 Version.h.in 生成的版本号头文件。
#include "Version.h"

// Qt Widgets 程序入口和系统托盘。
#include <QApplication>
#include <QSystemTrayIcon>

// Qt 路径/目录工具。当前文件里暂时没有直接使用，后续可考虑删除。
#include <QDir>

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QCoreApplication::setApplicationName("ZcChat2");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("MyOrganization");
    qInfo() << "Debugging Output";

    /*窗口创建*/
    Dialog dialogWin;
    // 对话框仍然提前创建，用于接收立绘信号和设置页配置；启动时不显示，右键立绘再打开。
    // dialogWin.show();
    Tachie tachieWin;
    tachieWin.show();
    MainWindow *settings = nullptr;

    /*一些绑定*/
    //对话框的开启和关闭
    QObject::connect(&tachieWin, &Tachie::requestToggleVisible, &dialogWin,
                     &Dialog::ToggleVisible);
    //修改立绘图片
    QObject::connect(&dialogWin, &Dialog::requestSetCharTachie, &tachieWin,
                     &Tachie::SetTachieImg);

    /*托盘*/
    QSystemTrayIcon tray;
    tray.setIcon(QIcon(":/res/img/logo/logo.png"));
    tray.setToolTip("ZcChat2");
    ElaMenu trayMenu;
    QAction *actionSettings = trayMenu.addAction("设置");
    QAction *actionQuit = trayMenu.addAction("退出");
    tray.setContextMenu(&trayMenu);
    tray.show();
    //左键点击托盘打开设置
    QObject::connect(&tray, QOverload<QSystemTrayIcon::ActivationReason>::of(&QSystemTrayIcon::activated),
                     [&](QSystemTrayIcon::ActivationReason reason)
                     {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                         {
                             if (!settings)
                             {
                                 eApp->init();
                                settings = new MainWindow(&dialogWin, &tachieWin);
                             }
                             settings->show();
                             settings->raise();
                             settings->activateWindow();
                         }
                     });
    //设置界面懒加载
    QObject::connect(actionSettings, &QAction::triggered, [&]()
                     {
                         if (!settings)
                         {
                             eApp->init();
                            settings = new MainWindow(&dialogWin, &tachieWin);
                         }
                         settings->show();
                         settings->raise();
                         settings->activateWindow(); });
    //退出程序
    QObject::connect(actionQuit, &QAction::triggered, &a, &QApplication::quit);

    return a.exec();
}
