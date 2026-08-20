#ifndef ALERTNOTIFIER_H
#define ALERTNOTIFIER_H

#include <QString>

namespace Service {

class AlertNotifier {
public:
    virtual ~AlertNotifier() = default;

    // Returns true when the notification was handed to an available delivery
    // channel. A false result lets AlertService retry instead of suppressing
    // the event as if it had already been shown.
    virtual bool showWeatherAlert(const QString& title, const QString& content) = 0;
};

} // namespace Service

#endif // ALERTNOTIFIER_H
