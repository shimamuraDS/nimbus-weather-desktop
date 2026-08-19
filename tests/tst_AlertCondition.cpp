#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include "util/WeatherCode.h"
#include "util/TimeUtil.h"

class tst_AlertCondition : public QObject {
    Q_OBJECT

private slots:
    void testSevereWeatherKeywords() {
        // 晴天、多云不触发提醒
        QVERIFY(!Util::WeatherCode::isSevereWeather(QString::fromUtf8("晴")));
        QVERIFY(!Util::WeatherCode::isSevereWeather(QString::fromUtf8("多云")));

        // 其他天气都触发提醒
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("阴")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("小雨")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("暴雨")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("特大暴雨")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("暴雪")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("雷阵雨伴有冰雹")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("沙尘暴")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("冻雨")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("多云转暴雨")));
        QVERIFY(Util::WeatherCode::isSevereWeather(QString::fromUtf8("晴转阴")));
    }

    void testTimeFormat() {
        QDateTime dt = Util::TimeUtil::parseTencentHour("2026-05-12 08:05");
        QVERIFY(dt.isValid());
        QCOMPARE(dt.date().year(), 2026);
        QCOMPARE(dt.date().month(), 5);
        QCOMPARE(dt.date().day(), 12);
        QCOMPARE(dt.time().hour(), 8);

        QString formatted = Util::TimeUtil::formatToHourlyString(dt);
        QVERIFY(formatted == QStringLiteral("2026-05-12 08:00"));
    }

    void testIsWithinFutureHours() {
        QDateTime future1Hour = QDateTime::currentDateTime().addSecs(1800);
        QVERIFY(Util::TimeUtil::isWithinFutureHours(future1Hour, 3));

        QDateTime future10Hours = QDateTime::currentDateTime().addSecs(36000);
        QVERIFY(!Util::TimeUtil::isWithinFutureHours(future10Hours, 3));

        QDateTime past1Hour = QDateTime::currentDateTime().addSecs(-3600);
        QVERIFY(!Util::TimeUtil::isWithinFutureHours(past1Hour, 3));
    }
};

QTEST_GUILESS_MAIN(tst_AlertCondition)
#include "tst_AlertCondition.moc"
