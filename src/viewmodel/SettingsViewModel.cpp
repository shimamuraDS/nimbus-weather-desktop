#include "SettingsViewModel.h"
#include "../util/Config.h"
#include "../network/GitHubReleaseClient.h"
#include <QDesktopServices>
#include <QUrl>
#ifdef WITH_LLM
#include "../llm/LLMClient.h"
#endif

namespace ViewModel {

SettingsViewModel::SettingsViewModel(Service::LocationService* locationService,
                                     QObject* parent)
    : QObject(parent), m_locationService(locationService) {}

bool SettingsViewModel::isAutoStart() const {
    return Util::Config::getInstance().isAutoStart();
}

void SettingsViewModel::setAutoStart(bool autoStart) {
    Util::Config::getInstance().setAutoStart(autoStart);
    emit settingsChanged();
}

QString SettingsViewModel::weatherApiKey() const {
    return Util::Config::getInstance().getWeatherApiKey();
}

void SettingsViewModel::setWeatherApiKey(const QString& key) {
    if (key == weatherApiKey()) return;
    Util::Config::getInstance().setWeatherApiKey(key);
    emit settingsChanged();
    emit weatherApiKeyChanged();
}

bool SettingsViewModel::isAutoLocation() const {
    return Util::Config::getInstance().isAutoLocation();
}

QString SettingsViewModel::manualCityName() const {
    return Util::Config::getInstance().getManualCityName();
}

QStringList SettingsViewModel::alertTimeList() const {
    return Util::Config::getInstance().getAlertTimes();
}

void SettingsViewModel::setAutoLocation(bool isAuto) {
    if (isAutoLocation() == isAuto) return;

    auto& config = Util::Config::getInstance();
    if (isAuto) {
        m_locationService->switchToAuto();
    } else {
        // Only switch mode setting — don't trigger weather refresh.
        // Weather will refresh when the user selects a city from CitySelector.
        config.setAutoLocation(false);
    }
    emit settingsChanged();
}

void SettingsViewModel::setManualCity(int adcode, const QString& name) {
    m_locationService->switchToManual(adcode, name);
    emit settingsChanged();
}

void SettingsViewModel::addAlertTime(const QString& time) {
    Util::Config::getInstance().addAlertTime(time);
    emit alertTimeListChanged();
}

void SettingsViewModel::removeAlertTime(const QString& time) {
    Util::Config::getInstance().removeAlertTime(time);
    emit alertTimeListChanged();
}

void SettingsViewModel::updateAlertTime(const QString& oldTime, const QString& newTime) {
    Util::Config::getInstance().updateAlertTime(oldTime, newTime);
    emit alertTimeListChanged();
}

QStringList SettingsViewModel::alertAdvanceList() const {
    return Util::Config::getInstance().getAlertAdvanceMinutes();
}

void SettingsViewModel::setAdvanceMinutes(const QString& alertTime, int minutes) {
    Util::Config::getInstance().setAdvanceMinutesFor(alertTime, minutes);
    emit alertTimeListChanged();
}

int SettingsViewModel::getAdvanceMinutesFor(const QString& alertTime) {
    return Util::Config::getInstance().getAdvanceMinutesFor(alertTime);
}

static bool isNewerVersion(const QString& latest, const QString& current)
{
    auto parse = [](const QString& v) -> QList<int> {
        QList<int> parts;
        QString s = v.startsWith('v') ? v.mid(1) : v;
        for (const QString& p : s.split('.')) {
            bool ok = false;
            int n = p.toInt(&ok);
            parts.append(ok ? n : 0);
        }
        return parts;
    };

    QList<int> latestParts = parse(latest);
    QList<int> currentParts = parse(current);

    int maxLen = qMax(latestParts.size(), currentParts.size());
    while (latestParts.size() < maxLen) latestParts.append(0);
    while (currentParts.size() < maxLen) currentParts.append(0);

    for (int i = 0; i < maxLen; ++i) {
        if (latestParts[i] > currentParts[i]) return true;
        if (latestParts[i] < currentParts[i]) return false;
    }
    return false;
}

void SettingsViewModel::checkForUpdates()
{
    auto* client = new Network::GitHubReleaseClient(this);
    connect(client, &Network::GitHubReleaseClient::releaseInfoFetched,
            this, [this, client](const QString& tagName, const QString& htmlUrl) {
        client->deleteLater();
        m_releaseUrl = htmlUrl;
        m_latestVersion = tagName;
        m_updateAvailable = isNewerVersion(tagName, NIMBUS_WEATHER_VERSION);
        emit updateInfoChanged();
    });
    connect(client, &Network::GitHubReleaseClient::errorOccurred,
            this, [this, client](const QString&) {
        client->deleteLater();
    });
    client->checkLatestRelease(QStringLiteral("shimamuraDS"),
                               QStringLiteral("nimbus-weather-desktop"));
}

void SettingsViewModel::openReleasePage()
{
    if (!m_releaseUrl.isEmpty())
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
    else
        QDesktopServices::openUrl(
            QUrl("https://github.com/shimamuraDS/nimbus-weather-desktop/releases"));
}

#ifdef WITH_LLM

bool SettingsViewModel::isLLMEnabled() const {
    return Util::Config::getInstance().isLLMEnabled();
}

void SettingsViewModel::setLLMEnabled(bool enabled) {
    Util::Config::getInstance().setLLMEnabled(enabled);
    emit llmSettingsChanged();
}

QString SettingsViewModel::llmApiUrl() const {
    return Util::Config::getInstance().getLLMApiUrl();
}

void SettingsViewModel::setLLMApiUrl(const QString& url) {
    Util::Config::getInstance().setLLMApiUrl(url);
    emit llmSettingsChanged();
}

QString SettingsViewModel::llmApiKey() const {
    return Util::Config::getInstance().getLLMApiKey();
}

void SettingsViewModel::setLLMApiKey(const QString& key) {
    Util::Config::getInstance().setLLMApiKey(key);
    emit llmSettingsChanged();
}

QString SettingsViewModel::llmModelName() const {
    return Util::Config::getInstance().getLLMModelName();
}

void SettingsViewModel::setLLMModelName(const QString& model) {
    Util::Config::getInstance().setLLMModelName(model);
    emit llmSettingsChanged();
}

void SettingsViewModel::testLLMConnection() {
    setLLMTestResult(QString::fromUtf8("连接中..."));
    auto& config = Util::Config::getInstance();
    auto* client = new LLM::LLMClient(this);
    client->chat(config.getLLMApiUrl(), config.getLLMApiKey(),
                 config.getLLMModelName(),
                 QString::fromUtf8("你好，请回复'连接成功'。"),
                 [this, client](const QString& text) {
        if (text.isEmpty())
            setLLMTestResult(QString::fromUtf8("连接失败"));
        else
            setLLMTestResult(QString::fromUtf8("连接成功"));
        client->deleteLater();
    }, 8000);
}

#endif

} // namespace ViewModel
