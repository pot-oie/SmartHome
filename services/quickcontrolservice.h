#pragma once

#include "database/dao/QuickControlDao.h"
#include <QString>
#include <QList>

// 专为 UI 层定义的展示结构体 (DTO)
struct QuickControlDisplayItem
{
    long long mappingId;    // quick_controls 表的主键
    QString targetType;     // "device" 或 "scene"
    long long targetId;     // 关联的底层自增 ID
    QString targetStringId; // 业务ID (device_id 或 scene_code)，用于调用控制命令
    QString displayName;    // 设备或场景名称
    QString iconPath;       // 图标路径
    bool isOnline;          // 设备是否在线 (仅 device 有效，scene 始终为 true)
    bool isOn;              // 设备是否开启 (仅 device 有效，scene 始终为 false)
};

// 管理用结构体
struct QuickControlManageItem
{
    QString targetType;
    long long targetId;
    QString displayName;
    bool isSelected; // 是否已经被选为快捷项
};

class QuickControlService
{
public:
    QuickControlService() = default;

    // 获取首页需要展示的所有快捷项（已完成设备和场景的自动组装）
    QList<QuickControlDisplayItem> getHomeShortcuts() const;

    // 添加快捷项 (给设置页的 UI 使用)
    bool addShortcut(const QString &targetType, long long targetId, QString *errorMessage = nullptr) const;

    // 移除快捷项 (给设置页的 UI 使用)
    bool removeShortcut(const QString &targetType, long long targetId, QString *errorMessage = nullptr) const;

    // 统一执行接口：帮助 UI 屏蔽底层差异。UI 点击后直接调这个函数
    // 如果是设备，targetState 决定开关；如果是场景，targetState 会被忽略（直接触发）
    bool executeShortcut(const QuickControlDisplayItem &item,
                         bool targetState = true,
                         QString *errorMessage = nullptr,
                         QString *warningMessage = nullptr) const;

    // 获取所有可用的设备和场景，并标记它们是否已被选中
    QList<QuickControlManageItem> getAllManageableItems() const;

    // 批量保存勾选的快捷项（覆盖式更新）
    bool saveManageableItems(const QList<QuickControlManageItem> &items, QString *errorMessage = nullptr) const;
};