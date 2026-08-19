#include "GitHubReleaseClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>
#include <memory>

namespace Network {

GitHubReleaseClient::GitHubReleaseClient(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

void GitHubReleaseClient::checkLatestRelease(const QString& repoOwner, const QString& repoName)
{
    QUrl url(QString("https://api.github.com/repos/%1/%2/releases/latest")
             .arg(repoOwner, repoName));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "NimbusWeather/1.0");
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_manager->get(request);
    auto timedOut = std::make_shared<bool>(false);
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply, timedOut]() {
        *timedOut = true;
        reply->abort();
    });
    timer->start(10000);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, timedOut]() {
        timer->stop();
        reply->deleteLater();

        if (*timedOut) {
            qDebug() << "[GitHubReleaseClient] Request timed out";
            emit errorOccurred(QStringLiteral("Request timed out"));
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[GitHubReleaseClient] Network error:" << reply->errorString();
            emit errorOccurred(reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "[GitHubReleaseClient] JSON parse error";
            emit errorOccurred(QStringLiteral("JSON parse error"));
            return;
        }

        QJsonObject obj = doc.object();
        QString tagName = obj["tag_name"].toString();
        QString htmlUrl = obj["html_url"].toString();

        if (tagName.isEmpty()) {
            emit errorOccurred(QStringLiteral("No tag_name in response"));
            return;
        }

        qDebug() << "[GitHubReleaseClient] Latest release:" << tagName;
        emit releaseInfoFetched(tagName, htmlUrl);
    });
}

} // namespace Network
