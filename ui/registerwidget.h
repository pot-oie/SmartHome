#pragma once

#include <QWidget>

#include "services/registerservice.h"

class QLineEdit;
class QPushButton;

class RegisterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterWidget(QWidget *parent = nullptr);

private slots:
    void onRegisterClicked();

private:
    void initUi();

private:
    RegisterService m_registerService;
    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_displayNameEdit = nullptr;
    QPushButton *m_registerButton = nullptr;
};
