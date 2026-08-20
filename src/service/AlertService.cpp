#include "AlertService.h"
#include "../util/Config.h"
#include "../util/TimeUtil.h"
#include "../util/WeatherCode.h"
#include "../data/WeatherCacheManager.h"
#ifdef WITH_LLM
#include "../llm/LLMAlertGenerator.h"
#endif
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

namespace Service {

AlertService::AlertService(AlertNotifier& notifier, QObject* parent)
    : QObject(parent), m_notifier(notifier) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AlertService::checkAlerts);
}

void AlertService::startMonitoring() {
    if (m_timer->isActive()) return;

    // Run once as soon as the event loop starts. Waiting a full minute after
    // launch can miss a short forecast window or an already-active warning.
    QTimer::singleShot(0, this, &AlertService::checkAlerts);
    m_timer->start(60000);
}

bool AlertService::checkSevereWeatherAlerts() {
    auto& cache = Data::WeatherCacheManager::getInstance();

    // Tier 1: official weather alarm always triggers
    QJsonArray alarms = cache.getCurrentAlarms();
    if (!alarms.isEmpty()) {
        QJsonObject alarm = alarms.first().toObject();
        QString alarmKey = alarm["title"].toString()
            + QLatin1Char('|') + alarm["pub_content"].toString();
        if (alarmKey == m_lastSevereAlertKey) return false;
        if (m_notifier.showWeatherAlert(alarm["title"].toString(),
                                        alarm["pub_content"].toString())) {
            m_lastSevereAlertKey = alarmKey;
            return true;
        } else {
            qWarning() << "[AlertService] Official alarm delivery failed; will retry";
        }
        return false;
    }

    QJsonArray hourlyData = cache.getHourlyData();
    if (hourlyData.isEmpty()) return false;

    // Find nearest non-sunny weather within the next 1 hour
    QString severeTime;
    QString severeDesc;
    QString severeEventKey;
    for (int i = 0; i < hourlyData.size(); ++i) {
        QJsonObject hourObj = hourlyData[i].toObject();
        QString hourStr = hourObj["hour"].toString();
        QDateTime dt = Util::TimeUtil::parseTencentHour(hourStr);

        if (Util::TimeUtil::isWithinFutureHours(dt, 1)) {
            QJsonObject info = hourObj["info"].toObject();
            QString weatherDesc = info["weather"].toString();

            if (Util::WeatherCode::isSevereWeather(weatherDesc)) {
                severeTime = hourStr.mid(11, 5);
                severeDesc = weatherDesc;
                severeEventKey = hourStr + QLatin1Char('|') + severeDesc;
                break;
            }
        }
    }

    if (severeTime.isEmpty()) {
        m_lastSevereAlertKey.clear();
        return false;
    }

    // Dedup: don't re-alert for the same weather event
    if (severeEventKey == m_lastSevereAlertKey) return false;
    // Get current weather
    QString currentHourKey = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:00");
    QString currentWeather;
    for (int i = 0; i < hourlyData.size(); ++i) {
        QJsonObject hourObj = hourlyData[i].toObject();
        if (hourObj["hour"].toString() == currentHourKey) {
            currentWeather = hourObj["info"].toObject()["weather"].toString();
            break;
        }
    }

    QString title = QString::fromUtf8("Nimbus Weather：未来1小时将有") + severeDesc;
    QString content = QString::fromUtf8("当前天气：") + currentWeather + "\n"
        + QString::fromUtf8("预计 ") + severeTime
        + QString::fromUtf8(" 左右将出现") + severeDesc
        + QString::fromUtf8("，请提前做好防范。");

    if (m_notifier.showWeatherAlert(title, content)) {
        m_lastSevereAlertKey = severeEventKey;
        return true;
    } else {
        qWarning() << "[AlertService] Severe-weather alert delivery failed; will retry";
    }
    return false;
}

void AlertService::checkAlerts() {
    auto& config = Util::Config::getInstance();
    QStringList alertTimes = config.getAlertTimes();

    // Severe-weather monitoring is always active. Scheduled forecast reminders
    // are an additional feature and must never disable safety alerts.
    const bool severeAlertDispatched = checkSevereWeatherAlerts();
    if (alertTimes.isEmpty()) return;

    // Alert times configured: alert at specified times for ALL weather (including sunny)
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    if (currentDateTime == m_lastAlertDateTime) return;
    QString currentTime = currentDateTime.mid(11, 5);
    if (!alertTimes.contains(currentTime)) return;

    // A newly dispatched safety alert already contains the actionable weather
    // information. Do not show a second scheduled notification in the same tick.
    if (severeAlertDispatched) return;

    // Get the duration window for this alert time
    QStringList durationMinutes = config.getAlertAdvanceMinutes();
    int idx = alertTimes.indexOf(currentTime);
    int durationMin = (idx >= 0 && idx < durationMinutes.size()) ? durationMinutes[idx].toInt() : 0;

    auto& cache = Data::WeatherCacheManager::getInstance();

    // Get current weather
    QJsonArray hourlyData = cache.getHourlyData();
    QString currentHourKey = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:00");
    QString currentWeather;
    for (int i = 0; i < hourlyData.size(); ++i) {
        QJsonObject hourObj = hourlyData[i].toObject();
        if (hourObj["hour"].toString() == currentHourKey) {
            currentWeather = hourObj["info"].toObject()["weather"].toString();
            break;
        }
    }

    // Check weather within the configured duration window (ALL weather types)
    int futureHours = qMax(1, (durationMin + 59) / 60);

    struct WeatherSegment {
        QString time;
        QString desc;
    };
    QList<WeatherSegment> segments;

    for (int i = 0; i < hourlyData.size(); ++i) {
        QJsonObject hourObj = hourlyData[i].toObject();
        QString hourStr = hourObj["hour"].toString();

        QDateTime dt = Util::TimeUtil::parseTencentHour(hourStr);

        if (Util::TimeUtil::isWithinFutureHours(dt, futureHours)) {
            QJsonObject info = hourObj["info"].toObject();
            QString weatherDesc = info["weather"].toString();
            segments.append({hourStr.mid(11, 5), weatherDesc});
        }
    }

    if (segments.isEmpty()) return;

    // Build fallback notification (neutral tone — covers sunny too)
    auto buildFallback = [currentWeather, durationMin, segments]() -> QPair<QString, QString> {
        QString title;
        QString content;
        content = QString::fromUtf8("当前天气：") + currentWeather + "\n";

        if (durationMin > 0) {
            int dH = durationMin / 60;
            int dM = durationMin % 60;
            QString durStr;
            if (dH > 0 && dM > 0)
                durStr = QString::number(dH) + QString::fromUtf8("小时") + QString::number(dM) + QString::fromUtf8("分钟");
            else if (dH > 0)
                durStr = QString::number(dH) + QString::fromUtf8("小时");
            else
                durStr = QString::number(dM) + QString::fromUtf8("分钟");

            title = QString::fromUtf8("Nimbus Weather：未来") + durStr + QString::fromUtf8("内天气");

            for (const auto& seg : segments) {
                content += seg.time + QString::fromUtf8("：") + seg.desc + "\n";
            }
        } else {
            const auto& seg = segments.first();
            title = QString::fromUtf8("Nimbus Weather：未来1小时天气");
            content += QString::fromUtf8("预计 ") + seg.time
                + QString::fromUtf8(" 左右为") + seg.desc;
        }
        return QPair<QString, QString>{title, content};
    };

#ifdef WITH_LLM
    if (Util::Config::getInstance().isLLMEnabled()) {
        auto* generator = new LLM::LLMAlertGenerator(this);
        generator->generateAlert(hourlyData, currentWeather, durationMin,
            [this, buildFallback, generator, currentDateTime](const QString& llmText) {
                bool delivered = false;
                if (llmText.isEmpty()) {
                    auto fb = buildFallback();
                    delivered = m_notifier.showWeatherAlert(fb.first, fb.second);
                } else {
                    delivered = m_notifier.showWeatherAlert(
                        QString::fromUtf8("Nimbus Weather"), llmText);
                }
                if (delivered) m_lastAlertDateTime = currentDateTime;
                generator->deleteLater();
            });
        return;
    }
#endif

    auto fb = buildFallback();
    if (m_notifier.showWeatherAlert(fb.first, fb.second)) {
        m_lastAlertDateTime = currentDateTime;
    }
}

} // namespace Service
