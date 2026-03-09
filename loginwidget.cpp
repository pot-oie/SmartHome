#include "loginwidget.h"
#include "ui_loginwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsDropShadowEffect>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    setWindowTitle("智能家居系统 - 登录");

    // 设置固定大小（对话框样式）
    setFixedSize(420, 520);

    // 设置窗口标志：无边框对话框
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // 移除透明背景属性以修复背景透明问题
    // setAttribute(Qt::WA_TranslucentBackground);

    // 居中显示窗口
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }

    // 设置图标
    ui->label_logo->setPixmap(QIcon(":/icons/home.svg").pixmap(80, 80));

    // 设置输入框样式
    ui->lineEdit_username->setPlaceholderText("请输入用户名");
    ui->lineEdit_password->setPlaceholderText("请输入密码");
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);

    // 设置默认账号密码（演示用）
    ui->lineEdit_username->setText("admin");
    ui->lineEdit_password->setText("123456");

    // 添加样式
    QString styleSheet = R"(
        QWidget#LoginWidget {
            background-color: white;
            border-radius: 20px;
        }
        
        QLabel#lblTitle {
            font-size: 24pt;
            font-weight: bold;
            color: #2196F3;
        }
        
        QLabel#lblSubtitle {
            font-size: 11pt;
            color: #666;
        }
        
        QLineEdit {
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 11pt;
            background-color: #f5f5f5;
        }
        
        QLineEdit:focus {
            border: 2px solid #2196F3;
            background-color: white;
        }
        
        QPushButton#btnLogin {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 12pt;
            font-weight: bold;
        }
        
        QPushButton#btnLogin:hover {
            background-color: #1976D2;
        }
        
        QPushButton#btnLogin:pressed {
            background-color: #0D47A1;
        }
        
        QPushButton#btnResetPwd {
            background-color: transparent;
            color: #2196F3;
            border: none;
            font-size: 10pt;
            text-decoration: underline;
        }
        
        QPushButton#btnResetPwd:hover {
            color: #1976D2;
        }
        
        QCheckBox {
            font-size: 10pt;
            color: #666;
        }
    )";

    setStyleSheet(styleSheet);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_btnLogin_clicked()
{
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    qDebug() << "登录按钮被点击，用户名：" << username;

    // 简单验证：用户名和密码不为空即可登录
    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "登录失败", "请输入用户名和密码！");
        return;
    }

    // 演示：admin/123456 可以登录
    if (username == "admin" && password == "123456")
    {
        QMessageBox::information(this, "登录成功", "欢迎使用智能家居监控平台！");
        emit loginSuccess();
    }
    else
    {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！\n\n提示：默认账号 admin / 123456");
    }
}

void LoginWidget::on_btnResetPwd_clicked()
{
    qDebug() << "重置密码按钮被点击";
    QMessageBox::information(this, "提示", "重置密码功能尚未实现\n\n"
                                           "在实际应用中，这里会发送验证码到\n"
                                           "注册的邮箱或手机号进行密码重置。");
}

bool LoginWidget::verifyUserInDatabase(const QString &username, const QString &password)
{
    Q_UNUSED(username);
    Q_UNUSED(password);
    // 预留数据库验证逻辑
    // TODO: 实际应用中应该查询数据库验证用户信息
    return false;
}
