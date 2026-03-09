#include "historyservice.h"

#include <QFile>
#include <QRandomGenerator>
#include <QStringConverter>
#include <QTextStream>
#include <QtMath>

OperationLogList HistoryService::queryOperationLogs(const QDate &startDate, const QDate &endDate) const
{
    OperationLogList logs;
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate)
    {
        return logs;
    }

    const QStringList users = {"admin", "张三", "李四"};
    const QStringList operations = {"设备控制", "场景激活", "参数调节", "系统设置"};
    const QStringList devices = {"客厅主灯", "客厅空调", "卧室窗帘", "智能门锁", "客厅电视"};
    const QStringList details = {
        "开启设备", "关闭设备", "调节温度至24°C",
        "调节亮度至80%", "激活回家模式", "修改报警阈值"};

    const QDateTime endDateTime(endDate, QTime(23, 59, 59));
    QDateTime current = endDateTime;

    while (current.date() >= startDate && logs.size() < 60)
    {
        OperationLogEntry entry;
        entry.timestamp = current;
        entry.user = users.at(QRandomGenerator::global()->bounded(users.size()));
        entry.operation = operations.at(QRandomGenerator::global()->bounded(operations.size()));
        entry.device = devices.at(QRandomGenerator::global()->bounded(devices.size()));
        entry.detail = details.at(QRandomGenerator::global()->bounded(details.size()));

        logs.push_back(entry);
        current = current.addSecs(-3600);
    }

    return logs;
}

EnvironmentSeries HistoryService::queryEnvironmentSeries(int hours) const
{
    EnvironmentSeries series;
    if (hours <= 0)
    {
        return series;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const double pi = 3.14159265358979323846;
    for (int i = hours - 1; i >= 0; --i)
    {
        EnvironmentPoint point;
        point.timestamp = now.addSecs(-i * 3600);

        // 用平滑趋势叠加轻微随机波动，便于图表展示
        const double trend = static_cast<double>(hours - i) / qMax(1, hours);
        point.temperature = 23.0 + 3.0 * qSin(trend * pi) + QRandomGenerator::global()->bounded(1.6);
        point.humidity = 45.0 + 8.0 * qCos(trend * pi) + QRandomGenerator::global()->bounded(2.4);

        series.push_back(point);
    }

    return series;
}

bool HistoryService::exportOperationLogsToCsv(const QString &filePath, const OperationLogList &logs, QString *errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (errorMessage)
        {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "时间,用户,操作类型,设备,详情\n";

    for (const OperationLogEntry &entry : logs)
    {
        out << '"' << entry.timestamp.toString("yyyy-MM-dd hh:mm:ss") << "\",";
        out << '"' << entry.user << "\",";
        out << '"' << entry.operation << "\",";
        out << '"' << entry.device << "\",";
        out << '"' << entry.detail << "\"\n";
    }

    return true;
}
