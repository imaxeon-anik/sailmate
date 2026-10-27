import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property string providerId
    property string providerTitle
    property string baseUrl: ""
    property string manualModel: ""
    property bool saving: false
    property bool loading: false
    property bool returnToConversation: false

    function submitKey() {
        if (chat.busy || vault.busy || !keyField.text.trim()) return
        recoveryNotice.text = ""
        chat.clearError()
        keyField.focus = false
        chat.validateKey(providerId, keyField.text.trim())
    }
    backNavigation: !chat.busy && !vault.busy
    allowedOrientations: Orientation.All
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        PullDownMenu {
            MenuItem {
                text: "Help"
                enabled: !chat.busy && !vault.busy
                onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"), { topic: "key" })
            }
            MenuItem {
                text: "Recover saved key"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    recoveryNotice.text = ""
                    chat.clearError()
                    keyField.focus = false
                    page.loading = true
                    vault.loadKey(page.providerId)
                }
            }
        }
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            PageHeader { title: manualModel ? "Save API key" : "Connect provider" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.providerTitle
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.baseUrl
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.WrapAnywhere
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Manual model: " + page.manualModel
                textFormat: Text.PlainText
                visible: page.manualModel.length > 0
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                    text: page.baseUrl.length > 0
                        ? "Your key and messages are sent to this endpoint."
                        : "Your key and messages are sent to this provider."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            PasswordField {
                id: keyField
                width: parent.width
                label: "API key"
                placeholderText: "Paste API key"
                enabled: !chat.busy && !vault.busy
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.submitKey()
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.manualModel.length > 0
                text: "Saves your key without verifying access."
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: page.manualModel ? "Save and continue" : "Connect"
                enabled: keyField.text.trim().length > 0 && !chat.busy && !vault.busy
                onClicked: page.submitKey()
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Cancel validation"
                visible: chat.busy && !vault.busy
                enabled: chat.busy && !vault.busy
                onClicked: chat.cancel()
            }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Medium
                running: chat.busy || vault.busy
                visible: running
            }
            Label {
                id: recoveryNotice
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: text.length > 0
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.error || vault.error
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
        }
        VerticalScrollDecorator {}
    }
    Connections {
        target: chat
        onKeyValidated: {
            if (page.status === PageStatus.Active && provider === page.providerId && !vault.busy) {
                page.saving = true
                vault.saveKey(provider, key)
            }
        }
    }
    Connections {
        target: vault
        onKeySaved: {
            if (provider === page.providerId && page.saving) {
                page.saving = false
                keyField.text = ""
                page.loading = true
                vault.loadKey(provider)
            }
        }
        onKeyLoaded: {
            if (page.status === PageStatus.Active && page.loading && provider === page.providerId && !chat.busy) {
                page.loading = false
                if (page.returnToConversation) {
                    chat.connectProvider(key)
                    if (chat.error.length === 0) pageStack.pop()
                } else {
                    if (page.providerId.indexOf("custom-") === 0) {
                        chat.showProviderHistory(provider)
                        if (!chat.error) app.showCustomHistory()
                    } else {
                        chat.activateProvider(provider, key)
                        if (chat.provider === provider && !chat.error) app.showChat()
                    }
                }
            }
        }
        onKeyMissing: {
            if (page.loading && provider === page.providerId) {
                page.loading = false
                recoveryNotice.text = "No saved key was found for this provider."
            }
        }
        onErrorChanged: { if (vault.error) { page.loading = false; page.saving = false } }
    }
    Component.onDestruction: keyField.text = ""
}
