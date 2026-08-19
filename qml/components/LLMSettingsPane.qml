import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    Layout.fillWidth: true
    implicitHeight: llmLayout.implicitHeight + theme.spacingLarge

    Theme { id: theme }

    // Preset data
    property var deepseekModels: [
        "deepseek-v4-flash",
        "deepseek-v4-pro",
        "deepseek-chat",
        "deepseek-reasoner"
    ]
    property var deepseekUrls: [
        "https://api.deepseek.com",
    ]

    property bool _providerReady: false

    ColumnLayout {
        id: llmLayout
        anchors.fill: parent
        anchors.margins: theme.spacingLarge
        spacing: theme.spacingMedium

        // Section label
        RowLayout {
            spacing: theme.spacingTiny
            Rectangle { width: 8; height: 8; radius: 4; color: theme.accentWarm }
            Text {
                text: qsTr("Nimbus Weather AI")
                font: theme.captionFont
                color: theme.secondaryText
            }

            // Help icon
            Rectangle {
                width: 16; height: 16; radius: 8
                color: "transparent"
                border.color: theme.secondaryText
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "?"
                    font.pixelSize: 11
                    color: theme.secondaryText
                }
                MouseArea {
                    id: aiHelpArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: Qt.openUrlExternally("https://platform.deepseek.com/api_keys")
                }
                ToolTip {
                    text: qsTr("如何获取？")
                    visible: aiHelpArea.containsMouse
                    delay: 500
                }
            }
        }

        // Enable toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: theme.spacingSmall
            Text {
                text: qsTr("启用 AI 生成提醒")
                font: theme.bodyFont
                color: theme.primaryText
                Layout.alignment: Qt.AlignVCenter
            }
            Item { Layout.fillWidth: true }
            Switch {
                id: llmToggle
                checked: typeof settingsViewModel !== "undefined" && settingsViewModel.llmEnabled
                onToggled: {
                    if (typeof settingsViewModel !== "undefined")
                        settingsViewModel.llmEnabled = checked
                }

                indicator: Rectangle {
                    implicitWidth: 42; implicitHeight: 22; radius: 11
                    color: llmToggle.checked ? theme.accentWarm : theme.cardBg
                    border.color: llmToggle.checked ? theme.accentWarm : theme.cardBorder
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 180 } }
                    Behavior on border.color { ColorAnimation { duration: 180 } }

                    Rectangle {
                        x: llmToggle.checked ? parent.width - width - 2 : 2
                        y: (parent.height - height) / 2
                        width: 18; height: 18; radius: 9
                        color: "#ffffff"
                        Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }
                    }
                }
            }
        }

        // Settings fields (visible only when enabled)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: theme.spacingSmall
            visible: llmToggle.checked
            opacity: llmToggle.checked ? 1 : 0

            // ── Model Provider Selection ──
            Text {
                text: qsTr("模型选择")
                font: theme.captionFont
                color: theme.secondaryText
            }
            ComboBox {
                id: providerCombo
                Layout.fillWidth: true
                implicitHeight: 34
                model: ["DeepSeek", qsTr("自定义")]
                textRole: "modelData"

                Component.onCompleted: {
                    var storedUrl = typeof settingsViewModel !== "undefined" ? settingsViewModel.llmApiUrl : ""
                    currentIndex = (storedUrl === "" || storedUrl === "https://api.deepseek.com" || storedUrl === "https://api.deepseek.com/anthropic") ? 0 : 1
                    root._providerReady = true
                }

                onCurrentIndexChanged: {
                    if (!root._providerReady) return
                    if (currentIndex === 0) {
                        apiUrlCombo.currentIndex = 0
                        modelCombo.currentIndex = 0
                        apiUrlCombo.commitApiUrl()
                        modelCombo.commitModelName()
                    } else {
                        apiUrlCombo.editText = ""
                        modelCombo.editText = ""
                        apiUrlCombo.commitApiUrl()
                        modelCombo.commitModelName()
                    }
                }

                contentItem: Text {
                    text: providerCombo.currentText
                    font: theme.bodyFont
                    color: theme.primaryText
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }

                background: Rectangle {
                    radius: theme.radiusSmall
                    color: providerCombo.pressed ? theme.cardBgHover :
                           (providerCombo.hovered ? theme.cardBgHover : theme.cardBg)
                    border.color: providerCombo.activeFocus ? theme.accentWarm :
                                  (providerCombo.hovered ? theme.cardBorderHover : theme.cardBorder)
                    border.width: 1
                }

                indicator: Rectangle {
                    x: providerCombo.width - 26
                    y: (providerCombo.height - 20) / 2
                    width: 20; height: 20; radius: 4
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "▾"
                        font.pixelSize: 14
                        color: theme.accentWarm
                    }
                    rotation: providerCombo.popup.visible ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 150 } }
                }

                popup: Popup {
                    id: providerPopup
                    y: providerCombo.height + 4
                    width: providerCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 4

                    Timer {
                        id: providerScrollTimer
                        interval: 1
                        onTriggered: {
                            if (providerListView.currentIndex >= 0) {
                                providerListView.positionViewAtIndex(providerListView.currentIndex, ListView.Beginning)
                            }
                        }
                    }

                    onOpened: { providerScrollTimer.start() }

                    contentItem: ListView {
                        id: providerListView
                        clip: true
                        implicitHeight: Math.min(contentHeight, 120)
                        model: providerCombo.popup.visible ? providerCombo.delegateModel : null
                        currentIndex: providerCombo.currentIndex
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall
                        color: "#1e1e42"
                        border.color: theme.cardBorderHover
                        border.width: 1
                    }
                }

                delegate: ItemDelegate {
                    width: providerCombo.popup.width - 8
                    implicitHeight: 40
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined

                    readonly property bool isSelected: providerCombo.currentIndex === index

                    contentItem: Text {
                        text: modelData
                        font: theme.bodyFont
                        color: parent.isSelected ? theme.accentWarm : (parent.hovered ? theme.accentWarm : theme.primaryText)
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall - 2
                        color: parent.isSelected ? theme.cardBgHover : (parent.hovered ? theme.cardBgHover : "transparent")
                    }

                    highlighted: isSelected || hovered
                }
            }

            // ── API URL (editable ComboBox) ──
            Text {
                text: qsTr("API 地址")
                font: theme.captionFont
                color: theme.secondaryText
            }
            ComboBox {
                id: apiUrlCombo
                Layout.fillWidth: true
                implicitHeight: 34
                editable: true
                model: providerCombo.currentIndex === 0 ? root.deepseekUrls : []
                textRole: "modelData"

                Component.onCompleted: {
                    var stored = typeof settingsViewModel !== "undefined" ? settingsViewModel.llmApiUrl : ""
                    if (stored !== "") {
                        editText = stored
                    } else if (providerCombo.currentIndex === 0) {
                        currentIndex = 0
                    }
                }

                onAccepted: commitApiUrl()
                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < (providerCombo.currentIndex === 0 ? root.deepseekUrls.length : 0))
                        commitApiUrl()
                }

                function commitApiUrl() {
                    if (typeof settingsViewModel !== "undefined")
                        settingsViewModel.llmApiUrl = editText
                }

                contentItem: TextInput {
                    text: apiUrlCombo.editText
                    font: theme.bodyFont
                    color: theme.primaryText
                    verticalAlignment: TextInput.AlignVCenter
                    leftPadding: 10
                    readOnly: false
                    clip: true
                    onEditingFinished: apiUrlCombo.commitApiUrl()
                }

                background: Rectangle {
                    radius: theme.radiusSmall
                    color: apiUrlCombo.pressed ? theme.cardBgHover :
                           (apiUrlCombo.hovered ? theme.cardBgHover : theme.cardBg)
                    border.color: apiUrlCombo.activeFocus ? theme.accentWarm :
                                  (apiUrlCombo.hovered ? theme.cardBorderHover : theme.cardBorder)
                    border.width: 1
                }

                indicator: Rectangle {
                    x: apiUrlCombo.width - 26
                    y: (apiUrlCombo.height - 20) / 2
                    width: 20; height: 20; radius: 4
                    color: "transparent"
                    visible: providerCombo.currentIndex === 0
                    Text {
                        anchors.centerIn: parent
                        text: "▾"
                        font.pixelSize: 14
                        color: theme.accentWarm
                    }
                    rotation: apiUrlCombo.popup.visible ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 150 } }
                }

                popup: Popup {
                    id: apiUrlPopup
                    y: apiUrlCombo.height + 4
                    width: apiUrlCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 4

                    Timer {
                        id: apiUrlScrollTimer
                        interval: 1
                        onTriggered: {
                            if (apiUrlListView.currentIndex >= 0) {
                                apiUrlListView.positionViewAtIndex(apiUrlListView.currentIndex, ListView.Beginning)
                            }
                        }
                    }

                    onOpened: {
                        var matched = -1
                        for (var i = 0; i < root.deepseekUrls.length; i++) {
                            if (apiUrlCombo.editText === root.deepseekUrls[i]) {
                                matched = i
                                break
                            }
                        }
                        if (matched >= 0) {
                            apiUrlCombo.currentIndex = matched
                        } else if (apiUrlCombo.editText === "" && providerCombo.currentIndex === 0) {
                            apiUrlCombo.currentIndex = 0
                        }
                        apiUrlScrollTimer.start()
                    }

                    contentItem: ListView {
                        id: apiUrlListView
                        clip: true
                        implicitHeight: Math.min(contentHeight, 100)
                        model: apiUrlCombo.popup.visible ? apiUrlCombo.delegateModel : null
                        currentIndex: apiUrlCombo.currentIndex
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall
                        color: "#1e1e42"
                        border.color: theme.cardBorderHover
                        border.width: 1
                    }
                }

                delegate: ItemDelegate {
                    width: apiUrlCombo.popup.width - 8
                    implicitHeight: 40
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined

                    readonly property bool isSelected: apiUrlCombo.currentIndex === index

                    contentItem: Text {
                        text: modelData
                        font: theme.bodyFont
                        color: parent.isSelected ? theme.accentWarm : (parent.hovered ? theme.accentWarm : theme.primaryText)
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideMiddle
                        leftPadding: 8
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall - 2
                        color: parent.isSelected ? theme.cardBgHover : (parent.hovered ? theme.cardBgHover : "transparent")
                    }

                    highlighted: isSelected || hovered
                }
            }

            // ── API Key ──
            Text {
                text: qsTr("API Key")
                font: theme.captionFont
                color: theme.secondaryText
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 34
                radius: theme.radiusSmall
                color: theme.cardBg
                border.color: apiKeyInput.activeFocus ? theme.accentWarm : theme.cardBorder
                border.width: 1

                TextInput {
                    id: apiKeyInput
                    anchors.fill: parent
                    anchors.margins: 8
                    text: typeof settingsViewModel !== "undefined" ? settingsViewModel.llmApiKey : ""
                    font: theme.bodyFont
                    color: theme.primaryText
                    echoMode: TextInput.Password
                    passwordCharacter: "•"
                    verticalAlignment: Text.AlignVCenter
                    onEditingFinished: {
                        if (typeof settingsViewModel !== "undefined")
                            settingsViewModel.llmApiKey = text
                    }
                }
            }

            // ── Model Name (editable ComboBox) ──
            Text {
                text: qsTr("模型名称")
                font: theme.captionFont
                color: theme.secondaryText
            }
            ComboBox {
                id: modelCombo
                Layout.fillWidth: true
                implicitHeight: 34
                editable: true
                model: providerCombo.currentIndex === 0 ? root.deepseekModels : []
                textRole: "modelData"

                Component.onCompleted: {
                    var stored = typeof settingsViewModel !== "undefined" ? settingsViewModel.llmModelName : ""
                    if (stored !== "") {
                        editText = stored
                    } else if (providerCombo.currentIndex === 0) {
                        currentIndex = 0
                    }
                }

                onAccepted: commitModelName()
                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < (providerCombo.currentIndex === 0 ? root.deepseekModels.length : 0))
                        commitModelName()
                }

                function commitModelName() {
                    if (typeof settingsViewModel !== "undefined")
                        settingsViewModel.llmModelName = editText
                }

                contentItem: TextInput {
                    text: modelCombo.editText
                    font: theme.bodyFont
                    color: theme.primaryText
                    verticalAlignment: TextInput.AlignVCenter
                    leftPadding: 10
                    readOnly: false
                    clip: true
                    onEditingFinished: modelCombo.commitModelName()
                }

                background: Rectangle {
                    radius: theme.radiusSmall
                    color: modelCombo.pressed ? theme.cardBgHover :
                           (modelCombo.hovered ? theme.cardBgHover : theme.cardBg)
                    border.color: modelCombo.activeFocus ? theme.accentWarm :
                                  (modelCombo.hovered ? theme.cardBorderHover : theme.cardBorder)
                    border.width: 1
                }

                indicator: Rectangle {
                    x: modelCombo.width - 26
                    y: (modelCombo.height - 20) / 2
                    width: 20; height: 20; radius: 4
                    color: "transparent"
                    visible: providerCombo.currentIndex === 0
                    Text {
                        anchors.centerIn: parent
                        text: "▾"
                        font.pixelSize: 14
                        color: theme.accentWarm
                    }
                    rotation: modelCombo.popup.visible ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 150 } }
                }

                popup: Popup {
                    id: modelPopup
                    y: modelCombo.height + 4
                    width: modelCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 4

                    Timer {
                        id: modelScrollTimer
                        interval: 1
                        onTriggered: {
                            if (modelListView.currentIndex >= 0) {
                                modelListView.positionViewAtIndex(modelListView.currentIndex, ListView.Beginning)
                            }
                        }
                    }

                    onOpened: {
                        var matched = -1
                        for (var i = 0; i < root.deepseekModels.length; i++) {
                            if (modelCombo.editText === root.deepseekModels[i]) {
                                matched = i
                                break
                            }
                        }
                        if (matched >= 0) {
                            modelCombo.currentIndex = matched
                        } else if (modelCombo.editText === "" && providerCombo.currentIndex === 0) {
                            modelCombo.currentIndex = 0
                        }
                        modelScrollTimer.start()
                    }

                    contentItem: ListView {
                        id: modelListView
                        clip: true
                        implicitHeight: Math.min(contentHeight, 220)
                        model: modelCombo.popup.visible ? modelCombo.delegateModel : null
                        currentIndex: modelCombo.currentIndex
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall
                        color: "#1e1e42"
                        border.color: theme.cardBorderHover
                        border.width: 1
                    }
                }

                delegate: ItemDelegate {
                    width: modelCombo.popup.width - 8
                    implicitHeight: 40
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined

                    readonly property bool isSelected: modelCombo.currentIndex === index

                    contentItem: Text {
                        text: modelData
                        font: theme.bodyFont
                        color: parent.isSelected ? theme.accentWarm : (parent.hovered ? theme.accentWarm : theme.primaryText)
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall - 2
                        color: parent.isSelected ? theme.cardBgHover : (parent.hovered ? theme.cardBgHover : "transparent")
                    }

                    highlighted: isSelected || hovered
                }
            }

            // Test connection
            RowLayout {
                Layout.topMargin: 2
                spacing: theme.spacingSmall

                Button {
                    id: testBtn
                    text: qsTr("测试连接")
                    font: theme.captionFont
                    flat: true
                    implicitHeight: 30

                    contentItem: Text {
                        text: testBtn.text
                        font: theme.captionFont
                        color: theme.accentWarm
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: theme.radiusSmall
                        color: testBtn.pressed ? theme.cardBgHover : (testBtn.hovered ? theme.cardBgHover : theme.cardBg)
                        border.color: testBtn.hovered ? theme.cardBorderHover : theme.cardBorder
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    onClicked: {
                        if (typeof settingsViewModel !== "undefined") {
                            apiUrlCombo.commitApiUrl()
                            settingsViewModel.llmApiKey = apiKeyInput.text
                            modelCombo.commitModelName()
                            settingsViewModel.testLLMConnection()
                        }
                    }
                }

                Text {
                    id: testResult
                    text: typeof settingsViewModel !== "undefined" ? settingsViewModel.llmTestResult : ""
                    font: theme.captionFont
                    color: text === qsTr("连接成功") ? theme.successText :
                           (text === qsTr("连接失败") ? theme.dangerText : theme.secondaryText)
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
