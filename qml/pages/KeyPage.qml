import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property string providerId
    property string providerTitle
    property bool saving: false
    backNavigation: !chat.busy && !vault.busy
    allowedOrientations: Orientation.All
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            PageHeader { title: "Connect " + providerTitle }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Enter your API key. It will be stored using Sailfish Secrets, not in your chat history. Validation checks access to the model list; chat availability also depends on billing and model permissions."
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            PasswordField {
                id: keyField
                width: parent.width
                label: "API key"
                placeholderText: "Paste API key"
                enabled: !chat.busy && !vault.busy
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Connect"
                enabled: keyField.text.trim().length > 0 && !chat.busy && !vault.busy
                onClicked: {
                    chat.clearError()
                    keyField.focus = false
                    chat.validateKey(providerId, keyField.text.trim())
                }
            }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Medium
                running: chat.busy || vault.busy
                visible: running
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.error || vault.error
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
            if (provider === page.providerId) {
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
                vault.loadKey(provider)
            }
        }
        onKeyLoaded: {
            if (provider === page.providerId) {
                chat.activateProvider(provider, key)
                app.showChat()
            }
        }
    }
    Component.onDestruction: keyField.text = ""
}
