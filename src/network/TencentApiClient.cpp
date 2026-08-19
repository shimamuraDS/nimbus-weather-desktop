#include "TencentApiClient.h"
#include "../util/Config.h"
#include <QUrlQuery>
#include <QDebug>

namespace Network {

namespace {

void setParseError(QString* error, const QString& message) {
    if (error) *error = message;
}

bool extractInfos(const QJsonObject& root, const QString& field,
                  QJsonArray& infos, QString* error) {
    const QJsonValue resultValue = root.value(QStringLiteral("result"));
    if (!resultValue.isObject()) {
        setParseError(error, QStringLiteral("Missing result object"));
        return false;
    }

    const QJsonValue containerValue = resultValue.toObject().value(field);
    if (!containerValue.isArray()) {
        setParseError(error, QStringLiteral("Missing %1 array").arg(field));
        return false;
    }

    const QJsonArray containers = containerValue.toArray();
    if (containers.isEmpty() || !containers.first().isObject()) {
        setParseError(error, QStringLiteral("Empty %1 array").arg(field));
        return false;
    }

    const QJsonValue infosValue = containers.first().toObject().value(QStringLiteral("infos"));
    if (!infosValue.isArray()) {
        setParseError(error, QStringLiteral("Missing %1 infos array").arg(field));
        return false;
    }

    infos = infosValue.toArray();
    return true;
}

} // namespace

TencentApiClient::TencentApiClient(QObject* parent) : HttpClient(parent) {}

QString TencentApiClient::getBaseUrl() const {
    return "https://apis.map.qq.com";
}

bool TencentApiClient::parseLocationResponse(const QJsonObject& root, int& cityAdcode,
                                             QString& city, QString* error) {
    const QJsonValue resultValue = root.value(QStringLiteral("result"));
    if (!resultValue.isObject()) {
        setParseError(error, QStringLiteral("Missing result object"));
        return false;
    }

    const QJsonValue adInfoValue = resultValue.toObject().value(QStringLiteral("ad_info"));
    if (!adInfoValue.isObject()) {
        setParseError(error, QStringLiteral("Missing ad_info object"));
        return false;
    }

    const QJsonObject adInfo = adInfoValue.toObject();
    const QJsonValue adcodeValue = adInfo.value(QStringLiteral("adcode"));
    bool adcodeOk = adcodeValue.isDouble();
    int rawAdcode = adcodeOk ? adcodeValue.toInt() : adcodeValue.toString().toInt(&adcodeOk);
    if (!adcodeOk || rawAdcode < 100000 || rawAdcode > 999999) {
        setParseError(error, QStringLiteral("Invalid adcode"));
        return false;
    }

    city = adInfo.value(QStringLiteral("city")).toString().trimmed();
    if (city.isEmpty()) {
        city = adInfo.value(QStringLiteral("district")).toString().trimmed();
    }
    if (city.isEmpty()) {
        setParseError(error, QStringLiteral("Missing city name"));
        return false;
    }

    const int prefix = rawAdcode / 10000;
    if (prefix == 11 || prefix == 12 || prefix == 31 || prefix == 50) {
        cityAdcode = prefix * 10000;
    } else {
        cityAdcode = (rawAdcode / 100) * 100;
    }
    return true;
}

bool TencentApiClient::parseHoursResponse(const QJsonObject& root, QJsonArray& hours,
                                          QString* error) {
    return extractInfos(root, QStringLiteral("forecast_hours"), hours, error);
}

bool TencentApiClient::parseFutureResponse(const QJsonObject& root, QJsonArray& forecast,
                                           QString* error) {
    return extractInfos(root, QStringLiteral("forecast"), forecast, error);
}

bool TencentApiClient::parseAlarmsResponse(const QJsonObject& root, QJsonArray& alarms,
                                           QString* error) {
    const QJsonValue resultValue = root.value(QStringLiteral("result"));
    if (!resultValue.isObject()) {
        setParseError(error, QStringLiteral("Missing result object"));
        return false;
    }

    const QJsonValue realtimeValue = resultValue.toObject().value(QStringLiteral("realtime"));
    if (!realtimeValue.isArray()) {
        setParseError(error, QStringLiteral("Missing realtime array"));
        return false;
    }

    const QJsonArray realtime = realtimeValue.toArray();
    if (realtime.isEmpty() || !realtime.first().isObject()) {
        setParseError(error, QStringLiteral("Empty realtime array"));
        return false;
    }

    const QJsonValue alarmsValue = realtime.first().toObject().value(QStringLiteral("alarms"));
    if (!alarmsValue.isUndefined() && !alarmsValue.isArray()) {
        setParseError(error, QStringLiteral("Invalid alarms field"));
        return false;
    }
    alarms = alarmsValue.toArray();
    return true;
}

void TencentApiClient::fetchLocation() {
    const QString apiKey = Util::Config::getInstance().getTencentApiKey().trimmed();
    if (apiKey.isEmpty()) {
        emit apiErrorOccurred(QStringLiteral("Location API"),
                              QStringLiteral("Weather API key is not configured"));
        return;
    }

    QUrl url(getBaseUrl() + "/ws/location/v1/ip");
    QUrlQuery query;
    query.addQueryItem("key", apiKey);
    url.setQuery(query);

    sendGetRequest(url, [this](const QJsonObject& root) {
        int cityAdcode = 0;
        QString city;
        QString error;
        if (!parseLocationResponse(root, cityAdcode, city, &error)) {
            emit apiErrorOccurred(QStringLiteral("Location API"), error);
            return;
        }

        emit locationFetched(cityAdcode, city);
    }, [this](const QString& err) {
        emit apiErrorOccurred("Location API", err);
    });
}

quint64 TencentApiClient::fetchAllWeatherInfo(int adcode) {
    QString apiKey = Util::Config::getInstance().getTencentApiKey().trimmed();
    if (apiKey.isEmpty()) {
        return 0;
    }
    if (adcode < 100000 || adcode > 999999) {
        return 0;
    }
    const quint64 requestId = ++m_nextWeatherRequestId;
    QString weatherUrl = getBaseUrl() + "/ws/weather/v1/";

    // 1. 未来24小时逐小时预报 (type=hours)
    {
        QUrl hoursUrl(weatherUrl);
        QUrlQuery hoursQuery;
        hoursQuery.addQueryItem("key", apiKey);
        hoursQuery.addQueryItem("adcode", QString::number(adcode));
        hoursQuery.addQueryItem("type", "hours");
        hoursUrl.setQuery(hoursQuery);

        sendGetRequest(hoursUrl, [this, requestId, adcode](const QJsonObject& root) {
            QJsonArray hoursInfos;
            QString error;
            if (!parseHoursResponse(root, hoursInfos, &error)) {
                emit weatherApiErrorOccurred(requestId, adcode,
                                             QStringLiteral("Weather-Hours API"), error);
                return;
            }
            emit hoursWeatherFetched(requestId, adcode, hoursInfos);
        }, [this, requestId, adcode](const QString& err) {
            emit weatherApiErrorOccurred(requestId, adcode,
                                         QStringLiteral("Weather-Hours API"), err);
        });
    }

    // 2. 未来7天预报 (type=future, get_md=1)
    {
        QUrl futureUrl(weatherUrl);
        QUrlQuery futureQuery;
        futureQuery.addQueryItem("key", apiKey);
        futureQuery.addQueryItem("adcode", QString::number(adcode));
        futureQuery.addQueryItem("type", "future");
        futureQuery.addQueryItem("get_md", "1");
        futureUrl.setQuery(futureQuery);

        sendGetRequest(futureUrl, [this, requestId, adcode](const QJsonObject& root) {
            QJsonArray futureInfos;
            QString error;
            if (!parseFutureResponse(root, futureInfos, &error)) {
                emit weatherApiErrorOccurred(requestId, adcode,
                                             QStringLiteral("Weather-Future API"), error);
                return;
            }
            emit futureWeatherFetched(requestId, adcode, futureInfos);
        }, [this, requestId, adcode](const QString& err) {
            emit weatherApiErrorOccurred(requestId, adcode,
                                         QStringLiteral("Weather-Future API"), err);
        });
    }

    // 3. 当前天气与预警 (type=now, added_fields=alarm)
    {
        QUrl nowUrl(weatherUrl);
        QUrlQuery nowQuery;
        nowQuery.addQueryItem("key", apiKey);
        nowQuery.addQueryItem("adcode", QString::number(adcode));
        nowQuery.addQueryItem("type", "now");
        nowQuery.addQueryItem("added_fields", "alarm");
        nowUrl.setQuery(nowQuery);

        sendGetRequest(nowUrl, [this, requestId, adcode](const QJsonObject& root) {
            QJsonArray alarms;
            QString error;
            if (!parseAlarmsResponse(root, alarms, &error)) {
                emit weatherApiErrorOccurred(requestId, adcode,
                                             QStringLiteral("Weather-Now(Alarm) API"), error);
                return;
            }
            emit alarmsFetched(requestId, adcode, alarms);
        }, [this, requestId, adcode](const QString& err) {
            emit weatherApiErrorOccurred(requestId, adcode,
                                         QStringLiteral("Weather-Now(Alarm) API"), err);
        });
    }

    return requestId;
}

} // namespace Network
