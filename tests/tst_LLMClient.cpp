#include <QtTest>
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "llm/LLMClient.h"

class tst_LLMClient : public QObject {
    Q_OBJECT

private slots:
    void testInvalidUrlFailsOnce() {
        LLM::LLMClient client;
        int callbackCount = 0;
        QString result = QStringLiteral("unexpected");

        client.chat(QStringLiteral("not-a-url"), QStringLiteral("key"),
                    QStringLiteral("model"), QStringLiteral("hello"),
                    [&](const QString& text) {
            ++callbackCount;
            result = text;
        });

        QCOMPARE(callbackCount, 1);
        QVERIFY(result.isEmpty());
    }

    void testTimeoutFailsOnce() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));

        LLM::LLMClient client;
        QEventLoop loop;
        int callbackCount = 0;
        QString result = QStringLiteral("unexpected");

        client.chat(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()),
                    QStringLiteral("key"), QStringLiteral("model"),
                    QStringLiteral("hello"),
                    [&](const QString& text) {
            ++callbackCount;
            result = text;
            loop.quit();
        }, 100);

        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();

        QCOMPARE(callbackCount, 1);
        QVERIFY(result.isEmpty());
        QTest::qWait(100);
        QCOMPARE(callbackCount, 1);
    }

    void testSuccessfulResponse() {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, &server, [&server]() {
            QTcpSocket* socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                socket->readAll();
                const QByteArray body = R"({"choices":[{"message":{"content":"ok"}}]})";
                const QByteArray response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " + QByteArray::number(body.size())
                    + "\r\n\r\n" + body;
                socket->write(response);
                socket->disconnectFromHost();
            });
        });

        LLM::LLMClient client;
        QEventLoop loop;
        int callbackCount = 0;
        QString result;
        client.chat(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()),
                    QStringLiteral("key"), QStringLiteral("model"),
                    QStringLiteral("hello"),
                    [&](const QString& text) {
            ++callbackCount;
            result = text;
            loop.quit();
        }, 1000);

        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();

        QCOMPARE(callbackCount, 1);
        QVERIFY(result == QStringLiteral("ok"));
    }
};

QTEST_GUILESS_MAIN(tst_LLMClient)
#include "tst_LLMClient.moc"
