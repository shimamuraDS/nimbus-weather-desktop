#include <QtTest>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include "data/WeatherCacheManager.h"
#include "service/AlertNotifier.h"
#include "service/AlertService.h"
#include "util/Config.h"
#include "util/TimeUtil.h"

namespace {

class FakeAlertNotifier final : public Service::AlertNotifier {
public:
    bool showWeatherAlert(const QString& title, const QString& content) override {
        ++callCount;
        lastTitle = title;
        lastContent = content;
        return deliverySucceeds;
    }

    bool deliverySucceeds = true;
    int callCount = 0;
    QString lastTitle;
    QString lastContent;
};

void invokeAlertCheck(Service::AlertService& service) {
    QVERIFY(QMetaObject::invokeMethod(&service, "checkAlerts", Qt::DirectConnection));
}

void prepareForecast(const QStringList& descriptions) {
    static int testAdcode = 990100;
    auto& cache = Data::WeatherCacheManager::getInstance();
    QVERIFY(cache.setActiveAdcode(++testAdcode));
    cache.updateCurrentAlarms(QJsonArray());

    QDateTime hour = QDateTime::currentDateTime();
    hour.setTime(QTime(hour.time().hour(), 0));

    QJsonArray records;
    for (qsizetype index = 0; index < descriptions.size(); ++index) {
        QJsonObject info;
        info["weather"] = descriptions[index];

        QJsonObject record;
        record["hour"] = hour.addSecs(index * 3600).toString(
            QStringLiteral("yyyy-MM-dd HH:mm"));
        record["info"] = info;
        records.append(record);
    }
    cache.appendHourlyData(records);
}

void prepareSevereForecast() {
    prepareForecast({QString::fromUtf8("晴"), QString::fromUtf8("小雨")});
}

} // namespace

class tst_AlertService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("NimbusWeatherTests"));
        QCoreApplication::setApplicationName(QStringLiteral("AlertServiceTests"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           QDir::tempPath() + QStringLiteral("/NimbusWeatherTests"));
        QSettings().clear();
    }

    void init() {
        auto& config = Util::Config::getInstance();
        config.setAlertTimes(QStringList());
        config.setAlertAdvanceMinutes(QStringList());
        prepareSevereForecast();
    }

    void cleanupTestCase() {
        QSettings().clear();
    }

    void notificationFailureIsRetried() {
        FakeAlertNotifier notifier;
        notifier.deliverySucceeds = false;
        Service::AlertService service(notifier);

        invokeAlertCheck(service);
        invokeAlertCheck(service);

        QCOMPARE(notifier.callCount, 2);
    }

    void successfulDeliveryIsDeduplicated() {
        FakeAlertNotifier notifier;
        Service::AlertService service(notifier);

        invokeAlertCheck(service);
        invokeAlertCheck(service);

        QCOMPARE(notifier.callCount, 1);
        QVERIFY(notifier.lastTitle.contains(QString::fromUtf8("未来1小时")));
    }

    void continuingSevereWeatherDoesNotAlertEveryHour() {
        prepareForecast({QString::fromUtf8("小雨"),
                         QString::fromUtf8("小雨"),
                         QString::fromUtf8("小雨")});

        FakeAlertNotifier notifier;
        Service::AlertService service(notifier);
        invokeAlertCheck(service);

        QCOMPARE(notifier.callCount, 0);
    }

    void scheduledReminderRunsOnlyAtConfiguredMinute() {
        auto& config = Util::Config::getInstance();
        const QString scheduledTime = QTime::currentTime().toString("HH:mm");
        config.setAlertTimes({scheduledTime});
        config.setAlertAdvanceMinutes({QStringLiteral("60")});
        prepareForecast({QString::fromUtf8("晴"), QString::fromUtf8("多云")});

        QCOMPARE(config.getAlertTimes(), QStringList{scheduledTime});
        const QJsonArray hourly =
            Data::WeatherCacheManager::getInstance().getHourlyData();
        QCOMPARE(hourly.size(), 2);
        const QDateTime nextHour = Util::TimeUtil::parseTencentHour(
            hourly[1].toObject()["hour"].toString());
        QVERIFY(Util::TimeUtil::isWithinFutureHours(nextHour, 1));

        FakeAlertNotifier notifier;
        Service::AlertService service(notifier);
        invokeAlertCheck(service);
        invokeAlertCheck(service);

        QCOMPARE(notifier.callCount, 1);
    }

    void scheduledReminderDoesNotDisableSevereMonitoring() {
        auto& config = Util::Config::getInstance();
        config.setAlertTimes({QTime::currentTime().addSecs(3600).toString("HH:mm")});
        config.setAlertAdvanceMinutes({QStringLiteral("60")});

        FakeAlertNotifier notifier;
        Service::AlertService service(notifier);
        invokeAlertCheck(service);

        QCOMPARE(notifier.callCount, 1);
        QVERIFY(notifier.lastTitle.contains(QString::fromUtf8("未来1小时")));
    }

    void monitoringChecksImmediately() {
        FakeAlertNotifier notifier;
        Service::AlertService service(notifier);

        service.startMonitoring();

        QTRY_COMPARE_WITH_TIMEOUT(notifier.callCount, 1, 500);
    }
};

QTEST_GUILESS_MAIN(tst_AlertService)
#include "tst_AlertService.moc"
