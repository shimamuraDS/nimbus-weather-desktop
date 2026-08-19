#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "network/HttpClient.h"
#include "network/TencentApiClient.h"

class TestHttpClient final : public Network::HttpClient {
public:
    using Network::HttpClient::sendGetRequest;
};

// Mock response JSONs (matching Tencent LBS API format)
static const char* MOCK_LOCATION_RESPONSE = R"({
    "status": 0,
    "message": "OK",
    "result": {
        "ad_info": {
            "adcode": 440305,
            "city": "深圳市"
        }
    }
})";

static const char* MOCK_HOURS_RESPONSE = R"({
    "status": 0,
    "result": {
        "forecast_hours": [{
            "infos": [
                {
                    "hour": "2026-05-12 08:00",
                    "info": {
                        "weather": "多云",
                        "temperature": 26,
                        "wind_direction": "东南风",
                        "wind_power": "2-3级"
                    }
                }
            ]
        }]
    }
})";

static const char* MOCK_FUTURE_RESPONSE = R"({
    "status": 0,
    "result": {
        "forecast": [{
            "infos": [
                {
                    "date": "2026-05-12",
                    "day": {"weather": "晴", "temperature": 28, "humidity": 45},
                    "night": {"weather": "多云", "temperature": 20, "humidity": 60}
                }
            ]
        }]
    }
})";

static const char* MOCK_NOW_ALARM_RESPONSE = R"({
    "status": 0,
    "result": {
        "realtime": [{
            "alarms": [
                {
                    "title": "深圳市发布暴雨黄色预警",
                    "pub_content": "预计未来6小时将出现暴雨",
                    "level_name": "黄色",
                    "type_name": "暴雨"
                }
            ]
        }]
    }
})";

class tst_HttpService : public QObject {
    Q_OBJECT

private slots:
    void testLocationResponseParsing() {
        QJsonDocument doc = QJsonDocument::fromJson(MOCK_LOCATION_RESPONSE);
        QVERIFY(!doc.isNull());
        int adcode = 0;
        QString city;
        QString error;
        QVERIFY2(Network::TencentApiClient::parseLocationResponse(
                     doc.object(), adcode, city, &error), qPrintable(error));
        QCOMPARE(adcode, 440300);
        QVERIFY(city == QString::fromUtf8("深圳市"));
    }

    void testHoursResponseParsing() {
        QJsonDocument doc = QJsonDocument::fromJson(MOCK_HOURS_RESPONSE);
        QVERIFY(!doc.isNull());

        QJsonArray infos;
        QString error;
        QVERIFY2(Network::TencentApiClient::parseHoursResponse(
                     doc.object(), infos, &error), qPrintable(error));
        QVERIFY(!infos.isEmpty());

        QJsonObject hour = infos[0].toObject();
        QVERIFY(hour["hour"].toString() == QStringLiteral("2026-05-12 08:00"));
        QVERIFY(hour["info"].toObject()["weather"].toString() == QString::fromUtf8("多云"));
        QCOMPARE(hour["info"].toObject()["temperature"].toInt(), 26);
    }

    void testFutureResponseParsing() {
        QJsonDocument doc = QJsonDocument::fromJson(MOCK_FUTURE_RESPONSE);
        QVERIFY(!doc.isNull());

        QJsonArray infos;
        QString error;
        QVERIFY2(Network::TencentApiClient::parseFutureResponse(
                     doc.object(), infos, &error), qPrintable(error));
        QVERIFY(!infos.isEmpty());

        QJsonObject day = infos[0].toObject();
        QVERIFY(day["date"].toString() == QStringLiteral("2026-05-12"));
        QVERIFY(day["day"].toObject()["weather"].toString() == QString::fromUtf8("晴"));
        QCOMPARE(day["day"].toObject()["temperature"].toInt(), 28);
        QCOMPARE(day["night"].toObject()["temperature"].toInt(), 20);
    }

    void testAlarmParsing() {
        QJsonDocument doc = QJsonDocument::fromJson(MOCK_NOW_ALARM_RESPONSE);
        QVERIFY(!doc.isNull());

        QJsonArray alarms;
        QString error;
        QVERIFY2(Network::TencentApiClient::parseAlarmsResponse(
                     doc.object(), alarms, &error), qPrintable(error));
        QVERIFY(!alarms.isEmpty());

        QJsonObject alarm = alarms[0].toObject();
        QVERIFY(alarm["title"].toString().contains(QString::fromUtf8("暴雨")));
        QVERIFY(!alarm["pub_content"].toString().isEmpty());
        QVERIFY(alarm["level_name"].toString() == QString::fromUtf8("黄色"));
    }

    void testMalformedResponseRejected() {
        QJsonArray output;
        QString error;
        QVERIFY(!Network::TencentApiClient::parseHoursResponse(
            QJsonObject{{QStringLiteral("result"), QJsonObject{}}}, output, &error));
        QVERIFY(!error.isEmpty());
    }

    void testHttpTimeoutCallsErrorOnce() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        TestHttpClient client;
        QEventLoop loop;
        int successCount = 0;
        int errorCount = 0;
        QString errorText;
        client.sendGetRequest(
            QUrl(QStringLiteral("http://127.0.0.1:%1/data?key=secret").arg(server.serverPort())),
            [&](const QJsonObject&) { ++successCount; },
            [&](const QString& error) {
                ++errorCount;
                errorText = error;
                loop.quit();
            },
            100);

        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();

        QCOMPARE(successCount, 0);
        QCOMPARE(errorCount, 1);
        QVERIFY(errorText == QStringLiteral("Request timed out"));
        QTest::qWait(100);
        QCOMPARE(errorCount, 1);
    }
};

QTEST_GUILESS_MAIN(tst_HttpService)
#include "tst_HttpService.moc"
