#include "scenewidget.h"
#include "ui_scenewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidgetItem>

SceneWidget::SceneWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
    loadScenesFromDatabase();
}

SceneWidget::~SceneWidget()
{
    delete ui;
}

void SceneWidget::loadScenesFromDatabase()
{
    // 从 SQLite 加载场景（使用假数据）
    ui->listWidget_scenes->clear();

    // 假数据：预设场景
    // 使用图标文件替代emoji
    struct SceneData
    {
        QString name;
        QString icon;
    };

    QVector<SceneData> scenes = {
        {"回家模式", ":/icons/home.svg"},
        {"睡眠模式", ":/icons/bedtime.svg"},
        {"影院模式", ":/icons/movie.svg"},
        {"派对模式", ":/icons/celebration.svg"},
        {"起床模式", ":/icons/wb_sunny.svg"},
        {"离家模式", ":/icons/flight_takeoff.svg"}};

    for (const auto &scene : scenes)
    {
        QListWidgetItem *item = new QListWidgetItem(scene.name);
        item->setIcon(QIcon(scene.icon));
        ui->listWidget_scenes->addItem(item);
    }

    // 默认选中第一个
    if (ui->listWidget_scenes->count() > 0)
    {
        ui->listWidget_scenes->setCurrentRow(0);
        updateSceneDetails(0);
    }

    // 连接场景选择信号
    connect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);
}

void SceneWidget::updateSceneDetails(int row)
{
    // 更新场景详情（假数据）
    ui->tableWidget_devices->clearContents();
    ui->tableWidget_devices->setRowCount(0);

    QStringList devices;
    QString description;

    switch (row)
    {
    case 0: // 回家模式
        devices << "客厅主灯,开启,80%"
                << "客厅窗帘,开启,100%"
                << "客厅空调,开启,24°C"
                << "前门智能锁,解锁,";
        description = "欢迎回家！自动开启照明、空调并打开窗帘";
        break;
    case 1: // 睡眠模式
        devices << "卧室灯,关闭,"
                << "客厅主灯,关闭,"
                << "卧室窗帘,关闭,0%"
                << "卧室空调,开启,26°C";
        description = "晚安！关闭所有照明，窗帘合上，空调调至舒适温度";
        break;
    case 2: // 影院模式
        devices << "客厅主灯,关闭,"
                << "客厅窗帘,关闭,0%"
                << "客厅电视,开启,";
        description = "享受电影时光！调暗灯光，关闭窗帘";
        break;
    case 3: // 派对模式
        devices << "客厅主灯,开启,100%"
                << "卧室灯,开启,80%"
                << "厨房灯,开启,100%"
                << "客厅电视,开启,";
        description = "派对时间！所有灯光全开";
        break;
    case 4: // 起床模式
        devices << "卧室灯,开启,60%"
                << "卧室窗帘,开启,80%"
                << "卧室空调,关闭,";
        description = "早安！缓缓开启灯光和窗帘，帮助您自然苏醒";
        break;
    case 5: // 离家模式
        devices << "所有灯光,关闭,"
                << "所有窗帘,关闭,"
                << "客厅空调,关闭,"
                << "卧室空调,关闭,"
                << "前门智能锁,上锁,"
                << "客厅摄像头,开启,监控";
        description = "安全离家！关闭所有电器，开启安防监控";
        break;
    default:
        description = "请选择一个场景";
    }

    ui->label_sceneDesc->setText(description);

    for (const QString &deviceStr : devices)
    {
        QStringList parts = deviceStr.split(',');
        if (parts.size() >= 2)
        {
            int row = ui->tableWidget_devices->rowCount();
            ui->tableWidget_devices->insertRow(row);
            ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(parts[0]));
            ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(parts[1]));
            ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(parts.size() > 2 ? parts[2] : ""));
        }
    }
}

void SceneWidget::updateSceneExecutionResult(const QJsonObject &resultData)
{
    // 处理执行结果
    qDebug() << "场景执行结果：" << resultData;
}

void SceneWidget::on_btnAddScene_clicked()
{
    qDebug() << "添加场景";

    bool ok;
    QString sceneName = QInputDialog::getText(this, "添加场景",
                                              "请输入场景名称：",
                                              QLineEdit::Normal, "", &ok);
    if (ok && !sceneName.isEmpty())
    {
        QListWidgetItem *item = new QListWidgetItem(sceneName);
        item->setIcon(QIcon(":/icons/scene.svg"));
        ui->listWidget_scenes->addItem(item);
        QMessageBox::information(this, "成功", "场景 \"" + sceneName + "\" 已添加！");
    }
}

void SceneWidget::on_btnDeleteScene_clicked()
{
    qDebug() << "删除场景";

    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (item)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认删除",
                                      "确定要删除场景 \"" + item->text() + "\" 吗？",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            delete item;
            QMessageBox::information(this, "成功", "场景已删除！");
        }
    }
    else
    {
        QMessageBox::warning(this, "提示", "请先选择要删除的场景！");
    }
}

void SceneWidget::on_btnActivateScene_clicked()
{
    qDebug() << "激活场景";

    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (item)
    {
        QMessageBox::information(this, "场景激活",
                                 "正在执行场景：" + item->text() + "\n\n" +
                                     "场景中的所有设备将按预设顺序执行操作...");
        // TODO: 实际应该发送 TCP 消息给后端
    }
    else
    {
        QMessageBox::warning(this, "提示", "请先选择要激活的场景！");
    }
}

void SceneWidget::on_btnAddDeviceToScene_clicked()
{
    qDebug() << "向场景添加设备";

    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (item)
    {
        QMessageBox::information(this, "提示",
                                 "在实际应用中，这里会弹出设备选择对话框，\n"
                                 "让您选择要添加到场景 \"" +
                                     item->text() + "\" 的设备及其操作。");
    }
    else
    {
        QMessageBox::warning(this, "提示", "请先选择一个场景！");
    }
}
