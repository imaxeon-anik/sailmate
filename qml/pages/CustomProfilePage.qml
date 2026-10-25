import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property bool creating: false
    backNavigation: !creating && !chat.busy && !vault.busy

    function createProfile() {
        if (creating || chat.busy || vault.busy || !nameField.text.trim() || !urlField.text.trim()) return
        creating = true
        chat.clearError()
        nameField.focus = false
        urlField.focus = false
        modelField.focus = false
        var provider = chat.createCustomProfile(nameField.text.trim(), urlField.text.trim(), modelField.text.trim())
        if (provider) {
            var profile = { name: nameField.text.trim(), baseUrl: urlField.text.trim(), model: modelField.text.trim() }
            for (var i = 0; i < chat.customProfiles.length; ++i) {
                if (chat.customProfiles[i].id === provider) {
                    profile = chat.customProfiles[i]
                    break
                }
            }
            pageStack.replace(Qt.resolvedUrl("KeyPage.qml"), {
                providerId: provider,
                providerTitle: profile.name,
                baseUrl: profile.baseUrl,
                manualModel: profile.model
            })
        } else {
            creating = false
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            PageHeader { title: "Add custom profile" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "OpenAI-compatible streaming chat completions with Bearer authentication. Only use an endpoint you trust and are authorised to use: your API key and conversation will be sent there."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            TextField {
                id: nameField
                width: parent.width
                label: "Profile name"
                placeholderText: "Name this connection"
                enabled: !page.creating && !chat.busy && !vault.busy
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: urlField.focus = true
            }
            TextField {
                id: urlField
                width: parent.width
                label: "HTTPS base URL"
                placeholderText: "https://api.example.org/v1"
                enabled: !page.creating && !chat.busy && !vault.busy
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: modelField.focus = true
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Include /v1 if your service requires it. Sailmate appends /models or /chat/completions. HTTPS only; no user information, query or fragment. Azure deployment URLs, query authentication and custom headers are not supported."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            TextField {
                id: modelField
                width: parent.width
                label: "Manual model ID (optional)"
                placeholderText: "Leave empty to discover models"
                enabled: !page.creating && !chat.busy && !vault.busy
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "A manual model skips the /models request. The next step saves your key without checking authentication or model access; only a chat request can test them. Use the service's exact model ID."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Creating saves this profile before key entry. Name, URL and manual model cannot be edited or deleted. For endpoint changes, create a separate profile with its own key and history. Removing a saved key keeps the profile and history."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Create profile"
                enabled: nameField.text.trim().length > 0 && urlField.text.trim().length > 0 && !page.creating && !chat.busy && !vault.busy
                onClicked: page.createProfile()
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
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: page.creating || chat.busy || vault.busy
                visible: running
                size: BusyIndicatorSize.Medium
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}
