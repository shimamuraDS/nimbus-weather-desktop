#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include <QObject>
#include <QTimer>
#include <QString>

#include "AlertNotifier.h"

namespace Service {

class AlertService : public QObject {
    Q_OBJECT
public:
    explicit AlertService(AlertNotifier& notifier, QObject* parent = nullptr);
    void startMonitoring();

private slots:
    void checkAlerts();

private:
    bool checkSevereWeatherAlerts();

    AlertNotifier& m_notifier;
    QTimer* m_timer;
    QString m_lastAlertDateTime;
    QString m_lastOfficialAlarmKey;
    QString m_lastForecastAlertKey;
};

} // namespace Service

#endif // ALERTSERVICE_H
