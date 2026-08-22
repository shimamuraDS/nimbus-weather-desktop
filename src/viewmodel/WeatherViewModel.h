#ifndef WEATHERVIEWMODEL_H
#define WEATHERVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QStringList>
#include "../service/WeatherService.h"
#include "../service/LocationService.h"

namespace ViewModel {

class WeatherViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentCity READ currentCity NOTIFY currentCityChanged)
    Q_PROPERTY(QVariantMap todayWeather READ todayWeather NOTIFY weatherDataChanged)
    Q_PROPERTY(QVariantList pastWeatherList READ pastWeatherList NOTIFY weatherDataChanged)
    Q_PROPERTY(QVariantList futureWeatherList READ futureWeatherList NOTIFY weatherDataChanged)
    Q_PROPERTY(QStringList alertTimeList READ alertTimeList NOTIFY alertTimeListChanged)
    Q_PROPERTY(bool isAutoLocation READ isAutoLocation NOTIFY locationModeChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isOffline READ isOffline NOTIFY isOfflineChanged)
    Q_PROPERTY(QVariantList hourlyList READ hourlyList NOTIFY hourlyDataChanged)

public:
    explicit WeatherViewModel(Service::WeatherService* weatherService,
                              Service::LocationService* locationService,
                              QObject* parent = nullptr);

    QString currentCity() const;
    QVariantMap todayWeather() const;
    QVariantList pastWeatherList() const;
    QVariantList futureWeatherList() const;
    QStringList alertTimeList() const;
    bool isAutoLocation() const;
    bool isLoading() const;
    bool isOffline() const;
    QVariantList hourlyList() const;

    Q_INVOKABLE void requestData();
    Q_INVOKABLE void refreshIfStale();
    Q_INVOKABLE void switchLocationMode();
    Q_INVOKABLE void addAlertTime(const QString& time);
    Q_INVOKABLE void removeAlertTime(const QString& time);

signals:
    void currentCityChanged();
    void weatherDataChanged();
    void alertTimeListChanged();
    void locationModeChanged();
    void isLoadingChanged();
    void isOfflineChanged();
    void hourlyDataChanged();

private slots:
    void onLocationChanged(int adcode, const QString& cityName);
    void onWeatherDataUpdated();

private:
    void loadFromCache();

    Service::WeatherService* m_weatherService;
    Service::LocationService* m_locationService;

    QString m_currentCity;
    QVariantMap m_todayWeather;
    QVariantList m_pastWeatherList;
    QVariantList m_futureWeatherList;
    QVariantList m_hourlyList;
    bool m_isLoading;
    bool m_isOffline;
};

} // namespace ViewModel

#endif // WEATHERVIEWMODEL_H
