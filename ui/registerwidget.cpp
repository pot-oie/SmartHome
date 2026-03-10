#include "registerwidget.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

RegisterWidget::RegisterWidget(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void RegisterWidget::initUi()
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QStringLiteral("用户注册"));
    setFixedSize(360, 240);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_displayNameEdit = new QLineEdit(this);
    m_displayNameEdit->setPlaceholderText(QStringLiteral("请输入显示名称"));

    m_registerButton = new QPushButton(QStringLiteral("注册"), this);
    connect(m_registerButton, &QPushButton::clicked, this, &RegisterWidget::onRegisterClicked);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(QStringLiteral("用户名"), m_usernameEdit);
    formLayout->addRow(QStringLiteral("密码"), m_passwordEdit);
    formLayout->addRow(QStringLiteral("显示名称"), m_displayNameEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(m_registerButton);
}

void RegisterWidget::onRegisterClicked()
{
    if (!m_registerService.registerUser(m_usernameEdit->text(),
                                        m_passwordEdit->text(),
                                        m_displayNameEdit->text()))
    {
        QMessageBox::warning(this,
                             QStringLiteral("注册失败"),
                             m_registerService.lastErrorMessage());
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("注册成功"),
                             QStringLiteral("用户已注册成功，请返回登录。"));
    close();
}
