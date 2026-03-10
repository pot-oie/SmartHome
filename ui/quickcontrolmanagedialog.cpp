#include "quickcontrolmanagedialog.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

QuickControlManageDialog::QuickControlManageDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("管理首页快捷控制");
    setMinimumSize(320, 450);

    m_listWidget = new QListWidget(this);
    QPushButton *btnSave = new QPushButton("保存", this);
    QPushButton *btnCancel = new QPushButton("取消", this);

    // 构建界面布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("请勾选要在首页显示的设备或场景：", this));
    mainLayout->addWidget(m_listWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, &QuickControlManageDialog::onSaveClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    // 从数据库加载数据并渲染成复选框列表
    m_items = m_service.getAllManageableItems();
    for (int i = 0; i < m_items.size(); ++i)
    {
        const auto &item = m_items[i];
        QListWidgetItem *listItem = new QListWidgetItem(m_listWidget);
        // 区分显示前缀
        QString prefix = (item.targetType == "device") ? "[设备] " : "[场景] ";
        listItem->setText(prefix + item.displayName);
        // 开启复选框功能
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(item.isSelected ? Qt::Checked : Qt::Unchecked);
        // 把索引存入 Data 以便保存时反查
        listItem->setData(Qt::UserRole, i);
    }
}

void QuickControlManageDialog::onSaveClicked()
{
    // 回写列表中的选中状态到结构体
    for (int row = 0; row < m_listWidget->count(); ++row)
    {
        QListWidgetItem *listItem = m_listWidget->item(row);
        int index = listItem->data(Qt::UserRole).toInt();
        m_items[index].isSelected = (listItem->checkState() == Qt::Checked);
    }

    QString errorMsg;
    if (m_service.saveManageableItems(m_items, &errorMsg))
    {
        accept(); // 保存成功，关闭弹窗
    }
    else
    {
        QMessageBox::warning(this, "保存失败", errorMsg);
    }
}