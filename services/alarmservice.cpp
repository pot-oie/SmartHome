#include "alarmservice.h"

#include <QRandomGenerator>

AlarmThreshold AlarmService::defaultThreshold() const
{
    return {18.0, 30.0, 30.0, 70.0};
}

AlarmLogList AlarmService::loadSampleLogs(int count) const
{
    AlarmLogList logs;
    if (count <= 0)
    {
        return logs;
    }

    const QStringList alarmTypes = {"温度过高", "温度过低", "湿度过高", "PM2.5超标", "CO2浓度过高", "设备离线"};
    const QStringList details = {
        "客厅温度达到32.5°C，超过设定上限",
        "卧室温度降至16.2°C，低于设定下限",
        "客厅湿度达到75%，超过设定上限",
        "客厅PM2.5浓度达到85，空气质量较差",
        "卧室CO2浓度达到1200ppm，建议开窗通风",
        "客厅摄像头连接异常，已离线"};

    for (int i = 0; i < count; ++i)
    {
        const int typeIndex = QRandomGenerator::global()->bounded(alarmTypes.size());
        AlarmLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime().addSecs(-i * 7200);
        entry.type = alarmTypes.at(typeIndex);
        entry.detail = details.at(typeIndex);
        logs.push_back(entry);
    }

    return logs;
}

AlarmLogEntry AlarmService::fromAlarmData(const QJsonObject &alarmData) const
{
    AlarmLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.type = alarmData.value("type").toString("未知报警");
    entry.detail = alarmData.value("message").toString("无详细信息");
    return entry;
}
