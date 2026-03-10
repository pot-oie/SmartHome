#include "loginwidget.h"
#include "ui_loginwidget.h"

#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("智能家居系统 - 登录"));

    setFixedSize(420, 520);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect screenGeometry = screen->geometry();
        const int x = (screenGeometry.width() - width()) / 2;
        const int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }

    ui->label_logo->setPixmap(QIcon(":/icons/home.svg").pixmap(80, 80));
    ui->lineEdit_username->setPlaceholderText(QStringLiteral("请输入用户名"));
    ui->lineEdit_password->setPlaceholderText(QStringLiteral("请输入密码"));
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_btnLogin_clicked()
{
    const QString username = ui->lineEdit_username->text().trimmed();
    const QString password = ui->lineEdit_password->text().trimmed();

    qDebug() << "登录按钮被点击，用户名:" << username;

    const LoginCheckResult result = m_loginService.checkCredential(username, password);
    if (result == LoginCheckResult::Success)
    {
        emit loginSuccess();
        return;
    }

    QMessageBox::warning(this, QStringLiteral("登录失败"), m_loginService.errorMessage(result));
}

void LoginWidget::on_btnResetPwd_clicked()
{
    qDebug() << "重置密码按钮被点击";
    QMessageBox::information(this,
                             QStringLiteral("提示"),
                             QStringLiteral("重置密码功能暂未实现。"));
}
