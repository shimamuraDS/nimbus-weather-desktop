#include "TrayViewModel.h"
#include <QCoreApplication>

namespace ViewModel {

TrayViewModel::TrayViewModel(QObject* parent)
    : QObject(parent),
      m_windowVisible(!QCoreApplication::arguments().contains(
          QStringLiteral("-hidden"), Qt::CaseInsensitive))
{}

bool TrayViewModel::windowVisible() const {
    return m_windowVisible;
}

void TrayViewModel::setWindowVisible(bool visible) {
    if (m_windowVisible != visible) {
        m_windowVisible = visible;
        emit windowVisibleChanged();
    }
}

void TrayViewModel::toggleWindow() {
    setWindowVisible(!m_windowVisible);
}

void TrayViewModel::showWindow() {
    setWindowVisible(true);
}

void TrayViewModel::hideWindow() {
    setWindowVisible(false);
}

void TrayViewModel::quitApp() {
    QCoreApplication::quit();
}

} // namespace ViewModel
