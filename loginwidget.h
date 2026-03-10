#pragma once

#include <QString>
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
    void loginSuccess();

private slots:
    void on_btnLogin_clicked();
    void on_btnRegister_clicked();
    void on_btnSkipLogin_clicked();

private:
    Ui::LoginWidget *ui;
    LoginService m_loginService;
};
