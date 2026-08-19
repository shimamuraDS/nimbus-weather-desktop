#include "WeatherViewModel.h"
#include "../data/WeatherCacheManager.h"
#include "../util/Config.h"
#include <QDate>
#include <QDebug>

namespace ViewModel {

WeatherViewModel::WeatherViewModel(Service::WeatherService* weatherService,
                                   Service::LocationService* locationService,
                                   QObject* parent)
    : QObject(parent),
      m_weatherService(weatherService),
      m_locationService(locationService),
      m_isLoading(false),
      m_isOffline(false)
{
    m_currentCity = Util::Config::getInstance().getManualCityName();

    connect(m_locationService, &Service::LocationService::locationChanged,
            this, &WeatherViewModel::onLocationChanged);

    connect(m_weatherService, &Service::WeatherService::weatherDataUpdated,
            this, &WeatherViewModel::onWeatherDataUpdated);
    connect(m_weatherService, &Service::WeatherService::weatherDataCleared,
            this, [this]() { loadFromCache(); });
    connect(m_weatherService, &Service::WeatherService::networkError,
            this, [this](const QString&) {
        if (!m_isOffline) {
            m_isOffline = true;
            emit isOfflineChanged();
        }
        if (m_isLoading) {
            m_isLoading = false;
            emit isLoadingChanged();
        }
    });

    loadFromCache();
}

QString WeatherViewModel::currentCity() const { return m_currentCity; }
QVariantMap WeatherViewModel::todayWeather() const { return m_todayWeather; }
QVariantList WeatherViewModel::pastWeatherList() const { return m_pastWeatherList; }
QVariantList WeatherViewModel::futureWeatherList() const { return m_futureWeatherList; }
QVariantList WeatherViewModel::hourlyList() const { return m_hourlyList; }
bool WeatherViewModel::isAutoLocation() const { return Util::Config::getInstance().isAutoLocation(); }
bool WeatherViewModel::isLoading() const { return m_isLoading; }
bool WeatherViewModel::isOffline() const { return m_isOffline; }

QStringList WeatherViewModel::alertTimeList() const {
    return Util::Config::getInstance().getAlertTimes();
}

void WeatherViewModel::requestData() {
    if (m_isLoading) return;

    m_isLoading = true;
    emit isLoadingChanged();

    m_locationService->initLocation();
}

void WeatherViewModel::switchLocationMode() {
    bool isAuto = isAutoLocation();
    auto& config = Util::Config::getInstance();
    if (isAuto) {
        m_locationService->switchToManual(config.getManualAdcode(),
                                          config.getManualCityName());
    } else {
        m_locationService->switchToAuto();
    }
    emit locationModeChanged();
}

void WeatherViewModel::addAlertTime(const QString& time) {
    Util::Config::getInstance().addAlertTime(time);
    emit alertTimeListChanged();
}

void WeatherViewModel::removeAlertTime(const QString& time) {
    Util::Config::getInstance().removeAlertTime(time);
    emit alertTimeListChanged();
}

void WeatherViewModel::onLocationChanged(int adcode, const QString& cityName) {
    if (m_currentCity != cityName) {
        m_currentCity = cityName;
        emit currentCityChanged();
    }
    m_weatherService->refreshWeatherData(adcode);
}

void WeatherViewModel::onWeatherDataUpdated() {
    loadFromCache();
    m_isLoading = false;
    emit isLoadingChanged();
    if (m_isOffline) {
        m_isOffline = false;
        emit isOfflineChanged();
    }
}

void WeatherViewModel::loadFromCache() {
    auto& cache = Data::WeatherCacheManager::getInstance();

    // 先清空
    m_todayWeather.clear();
    m_pastWeatherList.clear();
    m_futureWeatherList.clear();
    m_hourlyList.clear();

    QDate today = QDate::currentDate();

    // 加载过去7天
    QList<Data::DailyWeather> pastList = cache.getPastSevenDays();
    if (pastList.isEmpty()) {
        for (int i = 6; i >= 0; --i) {
            QVariantMap map;
            map["date"] = today.addDays(-i).toString("MM-dd");
            map["dayWeather"] = "--";
            map["dayTemp"] = 0;
            map["nightWeather"] = "--";
            map["nightTemp"] = 0;
            m_pastWeatherList.append(map);
        }
    } else {
        for (const auto& dw : pastList) {
            QVariantMap map;
            map["date"] = dw.date;
            map["dayWeather"] = dw.dayWeather;
            map["dayTemp"] = dw.dayTemp;
            map["nightWeather"] = dw.nightWeather;
            map["nightTemp"] = dw.nightTemp;
            m_pastWeatherList.append(map);
        }
    }

    // 加载未来7天及当日
    QList<Data::DailyWeather> futureList = cache.getFutureForecast();
    if (futureList.isEmpty()) {
        for (int i = 0; i < 7; ++i) {
            QVariantMap map;
            map["date"] = today.addDays(i).toString("MM-dd");
            map["dayWeather"] = "--";
            map["dayTemp"] = 0;
            map["dayHumidity"] = 0;
            map["nightWeather"] = "--";
            map["nightTemp"] = 0;
            map["nightHumidity"] = 0;
            m_futureWeatherList.append(map);
            if (i == 0) m_todayWeather = map;
        }
    } else {
        for (int i = 0; i < futureList.size(); ++i) {
            const auto& dw = futureList[i];
            QVariantMap map;
            map["date"] = dw.date;
            map["dayWeather"] = dw.dayWeather;
            map["dayTemp"] = dw.dayTemp;
            map["dayHumidity"] = dw.dayHumidity;
            map["nightWeather"] = dw.nightWeather;
            map["nightTemp"] = dw.nightTemp;
            map["nightHumidity"] = dw.nightHumidity;
            m_futureWeatherList.append(map);
            if (i == 0) m_todayWeather = map;
        }
    }

    // 加载逐小时数据（只保留今天的数据）
    QString todayStr = today.toString("yyyy-MM-dd");
    QJsonArray hourlyData = cache.getHourlyData();
    bool hasTodayHourly = false;
    for (const QJsonValue& val : hourlyData) {
        QJsonObject obj = val.toObject();
        QString hourStr = obj["hour"].toString();

        if (!hourStr.startsWith(todayStr))
            continue;

        hasTodayHourly = true;
        QJsonObject info = obj["info"].toObject();
        QString time = hourStr.length() >= 16 ? hourStr.mid(11, 5) : hourStr;

        QVariantMap map;
        map["time"] = time;
        map["weather"] = info["weather"].toString("");
        map["temperature"] = info["temperature"].toInt(0);
        m_hourlyList.append(map);
    }

    if (!hasTodayHourly) {
        for (int h = 0; h < 24; ++h) {
            QVariantMap map;
            map["time"] = QString("%1:00").arg(h, 2, 10, QChar('0'));
            map["weather"] = "--";
            map["temperature"] = 0;
            m_hourlyList.append(map);
        }
    }

    emit weatherDataChanged();
    emit hourlyDataChanged();
}

} // namespace ViewModel
