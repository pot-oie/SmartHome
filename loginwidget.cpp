#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "database/dao/UserDao.h"

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
    const QString username = ui->lineEdit_username->text().trimmed();
    const QString password = ui->lineEdit_password->text();

    qInfo() << "[LoginWidget] 收到登录请求，username =" << username;

    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "登录失败", "请输入用户名和密码。");
        return;
    }

    if (!verifyUserInDatabase(username, password))
    {
        QMessageBox::warning(this, "登录失败", "用户名不存在、密码错误，或用户已被禁用。");
        return;
    }

    QMessageBox::information(this, "登录成功", "欢迎使用智能家居监控平台。");
    emit loginSuccess();
}

void LoginWidget::on_btnResetPwd_clicked()
{
    qInfo() << "[LoginWidget] 点击了重置密码按钮。";
    QMessageBox::information(this, "提示", "重置密码功能当前阶段未实现。");
}

bool LoginWidget::verifyUserInDatabase(const QString &username, const QString &password)
{
    UserDao userDao;
    const bool success = userDao.verifyLogin(username, password);
    if (!success)
    {
        qWarning() << "[LoginWidget] 登录校验失败:" << userDao.lastErrorText();
    }

    return success;
}
