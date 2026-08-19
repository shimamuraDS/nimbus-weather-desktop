#ifndef TENCENTAPICLIENT_H
#define TENCENTAPICLIENT_H

#include "HttpClient.h"
#include <QJsonArray>

namespace Network {

class TencentApiClient : public HttpClient {
    Q_OBJECT
public:
    explicit TencentApiClient(QObject* parent = nullptr);

    void fetchLocation();
    quint64 fetchAllWeatherInfo(int adcode);

    static bool parseLocationResponse(const QJsonObject& root, int& cityAdcode,
                                      QString& city, QString* error = nullptr);
    static bool parseHoursResponse(const QJsonObject& root, QJsonArray& hours,
                                   QString* error = nullptr);
    static bool parseFutureResponse(const QJsonObject& root, QJsonArray& forecast,
                                    QString* error = nullptr);
    static bool parseAlarmsResponse(const QJsonObject& root, QJsonArray& alarms,
                                    QString* error = nullptr);

signals:
    void locationFetched(int adcode, const QString& city);
    void hoursWeatherFetched(quint64 requestId, int adcode, const QJsonArray& hoursInfos);
    void futureWeatherFetched(quint64 requestId, int adcode, const QJsonArray& futureInfos);
    void alarmsFetched(quint64 requestId, int adcode, const QJsonArray& alarms);
    void apiErrorOccurred(const QString& apiType, const QString& errorMessage);
    void weatherApiErrorOccurred(quint64 requestId, int adcode, const QString& apiType,
                                 const QString& errorMessage);

private:
    QString getBaseUrl() const;
    quint64 m_nextWeatherRequestId = 0;
};

} // namespace Network

#endif // TENCENTAPICLIENT_H
