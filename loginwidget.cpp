#include "loginwidget.h"
#include "ui/registerwidget.h"
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

void LoginWidget::on_btnRegister_clicked()
{
    qDebug() << "注册按钮被点击";

    RegisterWidget *registerWidget = new RegisterWidget(nullptr);
    registerWidget->show();
}

void LoginWidget::on_btnSkipLogin_clicked()
{
    qDebug() << "跳过登录(测试)按钮被点击";
    emit loginSuccess();
}
