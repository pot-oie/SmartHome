#include "settingswidget.h"
#include "ui_settingswidget.h"
#include <QDebug>

SettingsWidget::SettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);
    loadSystemSettings();
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::loadSystemSettings() {
    // 加载 QSettings
}

void SettingsWidget::on_cmbTheme_currentIndexChanged(int index) {
    qDebug() << "主题切换，索引：" << index;
}

void SettingsWidget::on_btnBackupDatabase_clicked() {
    qDebug() << "备份数据库";
}

void SettingsWidget::on_btnAddDevice_clicked() {
    qDebug() << "添加设备";
}

void SettingsWidget::on_btnDeleteDevice_clicked() {
    qDebug() << "删除设备";
}

void SettingsWidget::on_btnTestConnection_clicked() {
    qDebug() << "测试设备连通性";
}
