#include "historyservice.h"

#include "database/dao/HistoryDao.h"

#include <QFile>
#include <QIODevice>
#include <QStringList>
#include <QtConcurrent>

namespace
{
    struct ZipEntry
    {
        QByteArray fileName;
        QByteArray data;
        quint32 crc32 = 0;
        quint32 localHeaderOffset = 0;
    };

    quint32 crc32ForData(const QByteArray &data)
    {
        static quint32 table[256];
        static bool initialized = false;

        if (!initialized)
        {
            for (quint32 index = 0; index < 256; ++index)
            {
                quint32 value = index;
                for (int bit = 0; bit < 8; ++bit)
                {
                    if (value & 1U)
                    {
                        value = 0xEDB88320U ^ (value >> 1U);
                    }
                    else
                    {
                        value >>= 1U;
                    }
                }
                table[index] = value;
            }
            initialized = true;
        }

        quint32 crc = 0xFFFFFFFFU;
        for (unsigned char byte : data)
        {
            crc = table[(crc ^ byte) & 0xFFU] ^ (crc >> 8U);
        }
        return crc ^ 0xFFFFFFFFU;
    }

    bool writeUInt16(QIODevice *device, quint16 value)
    {
        const char bytes[2] = {
            static_cast<char>(value & 0xFF),
            static_cast<char>((value >> 8) & 0xFF)};
        return device->write(bytes, sizeof(bytes)) == sizeof(bytes);
    }

    bool writeUInt32(QIODevice *device, quint32 value)
    {
        const char bytes[4] = {
            static_cast<char>(value & 0xFF),
            static_cast<char>((value >> 8) & 0xFF),
            static_cast<char>((value >> 16) & 0xFF),
            static_cast<char>((value >> 24) & 0xFF)};
        return device->write(bytes, sizeof(bytes)) == sizeof(bytes);
    }

    QString escapeXmlText(const QString &text)
    {
        QString escaped;
        escaped.reserve(text.size());

        for (QChar ch : text)
        {
            const ushort unicode = ch.unicode();
            if (unicode < 0x20 && ch != QChar::fromLatin1('\t') && ch != QChar::fromLatin1('\n') && ch != QChar::fromLatin1('\r'))
            {
                continue;
            }

            switch (unicode)
            {
            case '&':
                escaped += QStringLiteral("&amp;");
                break;
            case '<':
                escaped += QStringLiteral("&lt;");
                break;
            case '>':
                escaped += QStringLiteral("&gt;");
                break;
            case '"':
                escaped += QStringLiteral("&quot;");
                break;
            case '\'':
                escaped += QStringLiteral("&apos;");
                break;
            default:
                escaped += ch;
                break;
            }
        }

        return escaped;
    }

    QString columnName(int columnIndex)
    {
        QString name;
        int value = columnIndex;
        while (value >= 0)
        {
            name.prepend(QChar::fromLatin1('A' + (value % 26)));
            value = value / 26 - 1;
        }
        return name;
    }

    QString inlineStringCell(int row, int column, const QString &text)
    {
        return QStringLiteral("<c r=\"%1%2\" t=\"inlineStr\"><is><t xml:space=\"preserve\">%3</t></is></c>")
            .arg(columnName(column), QString::number(row), escapeXmlText(text));
    }

    QByteArray buildWorksheetXml(const OperationLogList &logs)
    {
        QString xml = QStringLiteral(
                          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                          "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
                          "<dimension ref=\"A1:G%1\"/>"
                          "<sheetViews><sheetView workbookViewId=\"0\"/></sheetViews>"
                          "<sheetFormatPr defaultRowHeight=\"15\"/>"
                          "<cols>"
                          "<col min=\"1\" max=\"1\" width=\"12\" customWidth=\"1\"/>"
                          "<col min=\"2\" max=\"2\" width=\"22\" customWidth=\"1\"/>"
                          "<col min=\"3\" max=\"3\" width=\"16\" customWidth=\"1\"/>"
                          "<col min=\"4\" max=\"4\" width=\"18\" customWidth=\"1\"/>"
                          "<col min=\"5\" max=\"5\" width=\"20\" customWidth=\"1\"/>"
                          "<col min=\"6\" max=\"6\" width=\"42\" customWidth=\"1\"/>"
                          "<col min=\"7\" max=\"7\" width=\"14\" customWidth=\"1\"/>"
                          "</cols>"
                          "<sheetData>")
                          .arg(qMax(1, logs.size() + 1));

        const QStringList headers = {
            QStringLiteral("ID"),
            QStringLiteral("\u65f6\u95f4"),
            QStringLiteral("\u7528\u6237"),
            QStringLiteral("\u64cd\u4f5c\u7c7b\u578b"),
            QStringLiteral("\u8bbe\u5907"),
            QStringLiteral("\u8be6\u60c5"),
            QStringLiteral("\u7ed3\u679c")};

        xml += QStringLiteral("<row r=\"1\">");
        for (int column = 0; column < headers.size(); ++column)
        {
            xml += inlineStringCell(1, column, headers.at(column));
        }
        xml += QStringLiteral("</row>");

        for (int row = 0; row < logs.size(); ++row)
        {
            const OperationLogEntry &entry = logs.at(row);
            const int excelRow = row + 2;
            const QStringList cells = {
                QString::number(entry.recordId),
                entry.timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")),
                entry.user,
                entry.operation,
                entry.device,
                entry.detail,
                entry.result};

            xml += QStringLiteral("<row r=\"%1\">").arg(excelRow);
            for (int column = 0; column < cells.size(); ++column)
            {
                xml += inlineStringCell(excelRow, column, cells.at(column));
            }
            xml += QStringLiteral("</row>");
        }

        xml += QStringLiteral("</sheetData></worksheet>");
        return xml.toUtf8();
    }

    QByteArray buildWorkbookXml()
    {
        return QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
            "<sheets><sheet name=\"\u64cd\u4f5c\u65e5\u5fd7\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
            "</workbook>");
    }

    QByteArray buildWorkbookRelsXml()
    {
        return QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
            "Target=\"worksheets/sheet1.xml\"/>"
            "<Relationship Id=\"rId2\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
            "Target=\"styles.xml\"/>"
            "</Relationships>");
    }

    QByteArray buildRootRelsXml()
    {
        return QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
            "Target=\"xl/workbook.xml\"/>"
            "</Relationships>");
    }

    QByteArray buildStylesXml()
    {
        return QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
            "<fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
            "<fills count=\"2\">"
            "<fill><patternFill patternType=\"none\"/></fill>"
            "<fill><patternFill patternType=\"gray125\"/></fill>"
            "</fills>"
            "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
            "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
            "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs>"
            "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
            "</styleSheet>");
    }

    QByteArray buildContentTypesXml()
    {
        return QByteArray(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
            "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
            "<Override PartName=\"/xl/workbook.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
            "<Override PartName=\"/xl/worksheets/sheet1.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
            "<Override PartName=\"/xl/styles.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
            "</Types>");
    }

    bool writeZipFile(const QString &filePath, QVector<ZipEntry> entries, QString *errorMessage)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            if (errorMessage)
            {
                *errorMessage = file.errorString();
            }
            return false;
        }

        for (ZipEntry &entry : entries)
        {
            entry.crc32 = crc32ForData(entry.data);
            entry.localHeaderOffset = static_cast<quint32>(file.pos());

            if (!writeUInt32(&file, 0x04034B50U) || !writeUInt16(&file, 20) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt32(&file, entry.crc32) || !writeUInt32(&file, static_cast<quint32>(entry.data.size())) || !writeUInt32(&file, static_cast<quint32>(entry.data.size())) || !writeUInt16(&file, static_cast<quint16>(entry.fileName.size())) || !writeUInt16(&file, 0) || file.write(entry.fileName) != entry.fileName.size() || file.write(entry.data) != entry.data.size())
            {
                if (errorMessage)
                {
                    *errorMessage = file.errorString();
                }
                return false;
            }
        }

        const quint32 centralDirectoryOffset = static_cast<quint32>(file.pos());
        for (const ZipEntry &entry : entries)
        {
            if (!writeUInt32(&file, 0x02014B50U) || !writeUInt16(&file, 20) || !writeUInt16(&file, 20) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt32(&file, entry.crc32) || !writeUInt32(&file, static_cast<quint32>(entry.data.size())) || !writeUInt32(&file, static_cast<quint32>(entry.data.size())) || !writeUInt16(&file, static_cast<quint16>(entry.fileName.size())) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt32(&file, 0) || !writeUInt32(&file, entry.localHeaderOffset) || file.write(entry.fileName) != entry.fileName.size())
            {
                if (errorMessage)
                {
                    *errorMessage = file.errorString();
                }
                return false;
            }
        }

        const quint32 centralDirectorySize = static_cast<quint32>(file.pos()) - centralDirectoryOffset;
        if (!writeUInt32(&file, 0x06054B50U) || !writeUInt16(&file, 0) || !writeUInt16(&file, 0) || !writeUInt16(&file, static_cast<quint16>(entries.size())) || !writeUInt16(&file, static_cast<quint16>(entries.size())) || !writeUInt32(&file, centralDirectorySize) || !writeUInt32(&file, centralDirectoryOffset) || !writeUInt16(&file, 0))
        {
            if (errorMessage)
            {
                *errorMessage = file.errorString();
            }
            return false;
        }

        return true;
    }
}

OperationLogList HistoryService::queryOperationLogs(const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType) const
{
    HistoryDao dao;
    return dao.queryOperationLogs(startTime, endTime, deviceType);
}

EnvironmentSeries HistoryService::queryEnvironmentSeries(const QDateTime &startTime, const QDateTime &endTime) const
{
    HistoryDao dao;
    return dao.queryEnvironmentSeries(startTime, endTime);
}

bool HistoryService::addOperationLog(const QString &moduleName,
                                     const QString &operationType,
                                     const QString &operationContent,
                                     const QString &result,
                                     int resultCode,
                                     const QString &deviceId,
                                     const QString &deviceNameSnapshot,
                                     const QJsonObject &requestPayload,
                                     const QJsonObject &responsePayload,
                                     QString *errorMessage) const
{
    HistoryDao dao;
    if (!dao.insertOperationLog(moduleName,
                                operationType,
                                operationContent,
                                result,
                                resultCode,
                                deviceId,
                                deviceNameSnapshot,
                                requestPayload,
                                responsePayload))
    {
        if (errorMessage)
        {
            *errorMessage = dao.lastErrorText();
        }
        return false;
    }

    return true;
}

bool HistoryService::updateOperationLogResult(qint64 logId,
                                              const QString &result,
                                              int resultCode,
                                              const QJsonObject &responsePayload,
                                              QString *errorMessage) const
{
    HistoryDao dao;
    if (!dao.updateOperationLogResult(logId, result, resultCode, responsePayload))
    {
        if (errorMessage)
        {
            *errorMessage = dao.lastErrorText();
        }
        return false;
    }

    return true;
}

bool HistoryService::deleteOperationLog(qint64 logId, QString *errorMessage) const
{
    HistoryDao dao;
    if (!dao.deleteOperationLog(logId))
    {
        if (errorMessage)
        {
            *errorMessage = dao.lastErrorText();
        }
        return false;
    }

    return true;
}

bool HistoryService::exportOperationLogsToExcel(const QString &filePath, const OperationLogList &logs, QString *errorMessage) const
{
    const QVector<ZipEntry> entries = {
        {QByteArray("[Content_Types].xml"), buildContentTypesXml()},
        {QByteArray("_rels/.rels"), buildRootRelsXml()},
        {QByteArray("xl/workbook.xml"), buildWorkbookXml()},
        {QByteArray("xl/_rels/workbook.xml.rels"), buildWorkbookRelsXml()},
        {QByteArray("xl/worksheets/sheet1.xml"), buildWorksheetXml(logs)},
        {QByteArray("xl/styles.xml"), buildStylesXml()}};

    return writeZipFile(filePath, entries, errorMessage);
}

// ── 异步按需加载（构造/asyncQueryOperationLogs/asyncQueryEnvironmentSeries） ──────

HistoryService::HistoryService(QObject *parent)
    : QObject(parent)
{
}

void HistoryService::asyncQueryOperationLogs(
    const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType)
{
    if (!m_logWatcher)
    {
        m_logWatcher = new QFutureWatcher<OperationLogList>(this);
        connect(m_logWatcher, &QFutureWatcher<OperationLogList>::finished,
                this, &HistoryService::onLogWatcherFinished);
    }
    // 对同一个 watcher 重新设置 future 会自动断开上一个 future 的回调
    m_logWatcher->setFuture(QtConcurrent::run(
        [startTime, endTime, deviceType]() -> OperationLogList
        {
            HistoryService temp;
            return temp.queryOperationLogs(startTime, endTime, deviceType);
        }));
}

void HistoryService::asyncQueryEnvironmentSeries(const QDateTime &startTime, const QDateTime &endTime)
{
    if (!m_envWatcher)
    {
        m_envWatcher = new QFutureWatcher<EnvironmentSeries>(this);
        connect(m_envWatcher, &QFutureWatcher<EnvironmentSeries>::finished,
                this, &HistoryService::onEnvWatcherFinished);
    }
    m_envWatcher->setFuture(QtConcurrent::run(
        [startTime, endTime]() -> EnvironmentSeries
        {
            HistoryService temp;
            return temp.queryEnvironmentSeries(startTime, endTime);
        }));
}

void HistoryService::onLogWatcherFinished()
{
    emit operationLogsReady(m_logWatcher->result());
}

void HistoryService::onEnvWatcherFinished()
{
    emit environmentSeriesReady(m_envWatcher->result());
}
