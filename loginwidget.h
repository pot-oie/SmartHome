#pragma once
#include <QWidget>

#include "services/loginservice.h"

namespace Ui
{
    class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

signals:
    // 【向外发送】验证成功后，发射此信号通知 main.cpp 或 mainwindow 切换到主界面
    void loginSuccess();

private slots:
    // 【UI 交互】点击登录按钮触发
    void on_btnLogin_clicked();
    // 【UI 交互】点击重置密码触发
    void on_btnResetPwd_clicked();

private:
    Ui::LoginWidget *ui;
    LoginService m_loginService;
};
