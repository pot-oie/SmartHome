#include "mainwindow.h"
#include "loginwidget.h"

#include <QApplication>
#include <QLocale>
#include <QtGlobal>
#include <QTranslator>
#include <QFile>

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    qt_message_output(type, context, msg);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);

    // 加载全局样式表
    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly))
    {
        QString style = QLatin1String(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "SmartHome_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            a.installTranslator(&translator);
            break;
        }
    }

    // 创建主窗口和登录窗口
    MainWindow *mainWindow = new MainWindow();
    LoginWidget *loginWidget = new LoginWidget();

    // 连接登录成功信号：登录成功后关闭登录窗口，显示主窗口
    QObject::connect(loginWidget, &LoginWidget::loginSuccess, [mainWindow, loginWidget]()
                     {
        loginWidget->close();
        loginWidget->deleteLater();
        mainWindow->show(); });

    // 先显示登录窗口
    loginWidget->show();

    return a.exec();
}
