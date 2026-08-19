#include "WeatherService.h"
#include "../data/WeatherCacheManager.h"
#include <QDebug>

namespace Service {

WeatherService::WeatherService(QObject* parent) : QObject(parent) {
    m_apiClient = new Network::TencentApiClient(this);

    auto& cache = Data::WeatherCacheManager::getInstance();

    connect(m_apiClient, &Network::TencentApiClient::hoursWeatherFetched,
            this, [&cache, this](quint64 requestId, int adcode, const QJsonArray& hoursInfos) {
        if (requestId != m_currentRequestId || adcode != m_currentAdcode) return;
        qDebug() << "[WeatherService] Hours data received:" << hoursInfos.size() << "entries";
        cache.appendHourlyData(hoursInfos);
        completeRequest(true);
    });

    connect(m_apiClient, &Network::TencentApiClient::futureWeatherFetched,
            this, [&cache, this](quint64 requestId, int adcode, const QJsonArray& futureInfos) {
        if (requestId != m_currentRequestId || adcode != m_currentAdcode) return;
        qDebug() << "[WeatherService] Future data received:" << futureInfos.size() << "days";
        cache.updateFutureForecast(futureInfos);
        completeRequest(true);
    });

    connect(m_apiClient, &Network::TencentApiClient::alarmsFetched,
            this, [&cache, this](quint64 requestId, int adcode, const QJsonArray& alarms) {
        if (requestId != m_currentRequestId || adcode != m_currentAdcode) return;
        qDebug() << "[WeatherService] Alarms received:" << alarms.size() << "alarms";
        cache.updateCurrentAlarms(alarms);
        completeRequest(true);
    });

    connect(m_apiClient, &Network::TencentApiClient::weatherApiErrorOccurred,
            this, [this](quint64 requestId, int adcode,
                         const QString& apiType, const QString& errorMessage) {
        if (requestId != m_currentRequestId || adcode != m_currentAdcode) return;
        qWarning() << "[WeatherService] API Failed:" << apiType << "Reason:" << errorMessage;
        completeRequest(false, apiType + QStringLiteral(": ") + errorMessage);
    });
}

void WeatherService::completeRequest(bool succeeded, const QString& error) {
    if (m_pendingRequests <= 0) return;

    m_anyRequestSucceeded = m_anyRequestSucceeded || succeeded;
    if (!error.isEmpty()) m_requestErrors.append(error);
    --m_pendingRequests;
    if (m_pendingRequests > 0) return;

    if (m_anyRequestSucceeded) {
        emit weatherDataUpdated();
    } else {
        emit networkError(m_requestErrors.join(QLatin1Char('\n')));
    }
}

void WeatherService::refreshWeatherData(int adcode) {
    qDebug() << "[WeatherService] Fetching weather for adcode:" << adcode;
    m_currentAdcode = adcode;
    auto& cache = Data::WeatherCacheManager::getInstance();
    if (cache.setActiveAdcode(adcode)) {
        emit weatherDataCleared();
    }
    m_pendingRequests = 3;
    m_anyRequestSucceeded = false;
    m_requestErrors.clear();
    m_currentRequestId = m_apiClient->fetchAllWeatherInfo(adcode);
    if (m_currentRequestId == 0) {
        m_pendingRequests = 0;
        emit networkError(QStringLiteral("Weather request could not be started. Check the API key and city."));
    }
}

} // namespace Service
