#ifndef WEATHERCACHEMANAGER_H
#define WEATHERCACHEMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

namespace Data {

struct DailyWeather {
    QString date;
    QString dayWeather;
    int dayTemp = 0;
    int dayHumidity = 0;
    QString nightWeather;
    int nightTemp = 0;
    int nightHumidity = 0;
};

class WeatherCacheManager {
public:
    static WeatherCacheManager& getInstance();

    WeatherCacheManager(const WeatherCacheManager&) = delete;
    WeatherCacheManager& operator=(const WeatherCacheManager&) = delete;

    bool setActiveAdcode(int adcode);
    int getActiveAdcode() const;
    void appendHourlyData(const QJsonArray& forecastHoursInfos);
    void updateFutureForecast(const QJsonArray& futureInfos);
    void updateCurrentAlarms(const QJsonArray& alarms);

    QList<DailyWeather> getPastSevenDays();
    QList<DailyWeather> getFutureForecast();
    QJsonArray getCurrentAlarms();
    QJsonArray getHourlyData() const;

private:
    WeatherCacheManager();
    ~WeatherCacheManager() = default;

    QString m_cacheFilePath;
    QJsonObject m_cacheData;

    void loadCache();
    void saveCache();
    void cleanExpiredData();
};

} // namespace Data

#endif // WEATHERCACHEMANAGER_H
