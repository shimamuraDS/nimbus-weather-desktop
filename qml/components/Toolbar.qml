import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    Theme { id: theme }

    height: toolbarLayout.height
    color: "transparent"

    property bool isSettingsPage: false

    signal settingsClicked()
    signal backClicked()

    ColumnLayout {
        id: toolbarLayout
        width: parent.width
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: theme.spacingLarge
            Layout.rightMargin: theme.spacingMedium
            Layout.preferredHeight: 48

            Item {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.fill: parent
                    source: "qrc:/resources/icons/github.png"
                    fillMode: Image.PreserveAspectFit
                    opacity: ghMouseArea.containsMouse ? 1.0 : 0.65
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: -2
                    anchors.rightMargin: -2
                    width: 8; height: 8; radius: 4
                    color: "#ff3b30"
                    visible: typeof settingsViewModel !== "undefined" && settingsViewModel.updateAvailable
                }

                MouseArea {
                    id: ghMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof settingsViewModel !== "undefined" && settingsViewModel.updateAvailable) {
                            settingsViewModel.openReleasePage()
                        } else {
                            Qt.openUrlExternally("https://github.com/shimamuraDS/nimbus-weather-desktop")
                        }
                    }
                }
            }

            Item { width: 6; height: 1 }

            Text {
                text: qsTr("Nimbus Weather")
                font: theme.subtitleFont
                color: theme.primaryText
                Layout.alignment: Qt.AlignVCenter
            }

            Item { width: theme.spacingMedium; height: 1 }

            // ── Location section ──
            Text {
                visible: typeof settingsViewModel !== "undefined" && settingsViewModel.isAutoLocation
                text: typeof settingsViewModel !== "undefined"
                    ? (typeof weatherViewModel !== "undefined" ? weatherViewModel.currentCity : "")
                    : ""
                font: theme.captionFont
                color: theme.secondaryText
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                visible: typeof settingsViewModel !== "undefined" && settingsViewModel.isAutoLocation
                text: qsTr("定位不准？")
                font: theme.captionFont
                color: locLinkArea.containsMouse ? theme.accent : theme.accentWarm
                Layout.alignment: Qt.AlignVCenter

                MouseArea {
                    id: locLinkArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof settingsViewModel !== "undefined") {
                            settingsViewModel.setAutoLocation(false)
                        }
                    }
                }

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Text {
                visible: typeof settingsViewModel !== "undefined" && !settingsViewModel.isAutoLocation
                text: qsTr("城市：")
                font: theme.captionFont
                color: theme.secondaryText
                Layout.alignment: Qt.AlignVCenter
            }

            CitySelector {
                visible: typeof settingsViewModel !== "undefined" && !settingsViewModel.isAutoLocation
                Layout.preferredWidth: 90
                Layout.preferredHeight: 28
                onCitySelected: function(adcode, name) {
                    if (typeof settingsViewModel !== "undefined") {
                        settingsViewModel.setManualCity(adcode, name)
                    }
                }
            }

            Text {
                visible: typeof settingsViewModel !== "undefined" && !settingsViewModel.isAutoLocation
                text: qsTr("自动定位")
                font: theme.captionFont
                color: autoLocLinkArea.containsMouse ? theme.accent : theme.accentSecondary
                Layout.alignment: Qt.AlignVCenter

                MouseArea {
                    id: autoLocLinkArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof settingsViewModel !== "undefined") {
                            settingsViewModel.setAutoLocation(true)
                        }
                    }
                }

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: actionBtn
                text: root.isSettingsPage ? qsTr("◀ 返回") : qsTr("⚙ 设置")
                font: theme.bodyFont
                flat: true
                Layout.preferredHeight: 32

                contentItem: Text {
                    text: actionBtn.text
                    font: actionBtn.font
                    color: theme.primaryText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    rightPadding: 12
                }

                background: Rectangle {
                    radius: theme.radiusSmall
                    color: actionBtn.pressed ? theme.cardBgHover : (actionBtn.hovered ? theme.cardBg : "transparent")
                    border.color: actionBtn.hovered ? theme.cardBorder : "transparent"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }

                onClicked: {
                    if (root.isSettingsPage) {
                        root.backClicked()
                    } else {
                        root.settingsClicked()
                    }
                }
            }

            Button {
                id: minimizeBtn
                text: qsTr("−")
                font: theme.subtitleFont
                flat: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignVCenter

                contentItem: Text {
                    text: minimizeBtn.text
                    font: minimizeBtn.font
                    color: theme.primaryText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 16
                    color: minimizeBtn.pressed ? theme.cardBgHover : (minimizeBtn.hovered ? theme.cardBg : "transparent")
                    border.color: minimizeBtn.hovered ? theme.cardBorder : "transparent"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }

                onClicked: {
                    if (typeof trayViewModel !== "undefined") {
                        trayViewModel.hideWindow()
                    }
                }
            }
        }

        // Bottom divider for toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: theme.divider
        }

        // ── Offline warning banner ──
        Rectangle {
            Layout.fillWidth: true
            height: visible ? 28 : 0
            color: theme.dangerBg
            visible: typeof weatherViewModel !== "undefined" && weatherViewModel.isOffline
            clip: true

            Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.OutQuint } }

            RowLayout {
                anchors.centerIn: parent
                spacing: 6

                Text {
                    id: warningIcon
                    text: "⚠"
                    font: theme.bodyFont
                    color: theme.dangerText
                    transformOrigin: Item.Center

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: warningIcon.visible
                        NumberAnimation { from: 1.0; to: 1.25; duration: 800; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 1.25; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                    }
                }

                Text {
                    text: qsTr("网络连接异常，正在显示缓存数据")
                    font: theme.captionFont
                    color: theme.dangerText
                }
            }
        }
    }
}
