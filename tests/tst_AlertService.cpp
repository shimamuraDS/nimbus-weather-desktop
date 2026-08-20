#include <QtTest>
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include "data/WeatherCacheManager.h"
#include "service/AlertNotifier.h"
#include "service/AlertService.h"
#include "util/Config.h"

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

void prepareSevereForecast() {
    static int testAdcode = 990100;
    auto& cache = Data::WeatherCacheManager::getInstance();
    QVERIFY(cache.setActiveAdcode(++testAdcode));
    cache.updateCurrentAlarms(QJsonArray());

    QDateTime eventTime = QDateTime::currentDateTime().addSecs(3600);
    eventTime.setTime(QTime(eventTime.time().hour(), 0));

    QJsonObject info;
    info["weather"] = QString::fromUtf8("小雨");

    QJsonObject hour;
    hour["hour"] = eventTime.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    hour["info"] = info;
    cache.appendHourlyData(QJsonArray{hour});
}

} // namespace

class tst_AlertService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("NimbusWeatherTests"));
        QCoreApplication::setApplicationName(QStringLiteral("AlertServiceTests"));
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
