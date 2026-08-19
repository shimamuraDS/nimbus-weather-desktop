#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QObject>
#include <QStringList>
#include "../network/TencentApiClient.h"

namespace Service {

class WeatherService : public QObject {
    Q_OBJECT
public:
    explicit WeatherService(QObject* parent = nullptr);

    void refreshWeatherData(int adcode);

signals:
    void weatherDataCleared();
    void weatherDataUpdated();
    void networkError(const QString& message);

private:
    void completeRequest(bool succeeded, const QString& error = QString());

    Network::TencentApiClient* m_apiClient;
    quint64 m_currentRequestId = 0;
    int m_currentAdcode = 0;
    int m_pendingRequests = 0;
    bool m_anyRequestSucceeded = false;
    QStringList m_requestErrors;
};

} // namespace Service

#endif // WEATHERSERVICE_H
