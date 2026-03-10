#pragma once

#include <QString>
#include <QList>

// 对应 quick_controls 表的原始数据映射
struct QuickControlRow
{
    long long id;
    QString targetType; // 'device' 或 'scene'
    long long targetId; // 对应的设备ID或场景ID
    int displayOrder;
};

class QuickControlDao
{
public:
    QuickControlDao() = default;

    // 获取所有快捷控制项，按 display_order 升序排列
    QList<QuickControlRow> listAll() const;

    // 添加快捷项
    bool addShortcut(const QString &targetType, long long targetId) const;

    // 移除快捷项
    bool removeShortcut(const QString &targetType, long long targetId) const;

    // 清空所有快捷项
    bool clearAll() const;

    QString lastErrorText() const;

private:
    mutable QString m_lastErrorText;
};