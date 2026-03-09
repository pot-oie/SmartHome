#include "loginwidget.h"
#include "ui_loginwidget.h"
#include <QDebug>
#include <QMessageBox>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    setWindowTitle("智能家居系统 - 登录");
    resize(400, 300);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_btnLogin_clicked()
{
    QString username = ui->lineEdit_username->text();
    QString password = ui->lineEdit_password->text();

    qDebug() << "登录按钮被点击，用户名：" << username;

    // 简单验证：用户名和密码不为空即可登录（演示用）
    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "登录失败", "请输入用户名和密码！");
        return;
    }

    // TODO: 后续可以调用 verifyUserInDatabase 进行真实验证
    // if (verifyUserInDatabase(username, password)) {
    //     emit loginSuccess();
    // } else {
    //     QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
    // }

    // 演示：直接登录成功
    QMessageBox::information(this, "登录成功", "欢迎使用智能家居系统！");
    emit loginSuccess();
}

void LoginWidget::on_btnResetPwd_clicked()
{
    qDebug() << "重置密码按钮被点击";
    QMessageBox::information(this, "提示", "重置密码功能尚未实现");
}

bool LoginWidget::verifyUserInDatabase(const QString &username, const QString &password)
{
    // 预留数据库验证逻辑
    return false;
}
