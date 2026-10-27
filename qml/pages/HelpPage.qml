import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property string topic: "general"
    readonly property var sections: topic === "profile" ? [
        { title: "Compatible endpoints", body: "Use an OpenAI-compatible service with streaming chat completions and Bearer API-key authentication. Your key and conversation are sent to the chosen endpoint." },
        { title: "Base URL", body: "Include /v1 if required. Sailmate appends /models or /chat/completions. HTTPS is required; user information, query strings and fragments are not accepted. Azure deployment URLs, query authentication and custom headers are not supported." },
        { title: "Manual model", body: "Leave the model ID empty to discover models. A manual model skips /models and saves your key without verifying authentication, billing or model access. These are tested when you send a chat request. Use the service's exact model ID." },
        { title: "Profiles and history", body: "Creating saves the profile before key entry, even if you cancel that step. For endpoint changes, create a separate profile with its own key and history. Deleting a profile removes its saved key and local chats; removing only its key keeps the profile and history. Neither action revokes the key at the service." }
    ] : topic === "key" ? [
        { title: "Key storage", body: "API keys are stored using Sailfish Secrets, not in chat history. Configure a device lock and unlock the device when requested. There is no plaintext fallback." },
        { title: "Connection checks", body: "Connect checks access to the model list. A successful check does not guarantee chat availability, billing or model permissions. Consumer chat subscriptions do not necessarily include API usage; API charges may apply." },
        { title: "Manual models", body: "Save and continue stores your key without verifying access when a manual model is configured. The /models request is skipped; authentication, billing and model access are tested when you send a chat request." },
        { title: "Recover saved key", body: "Use the key page's Recover saved key pulley action to look for an existing Sailfish Secrets entry and repair its settings index when possible. This does not recover missing Custom profiles or change secret ownership." },
        { title: "Endpoint privacy", body: "Your key and messages go to the selected provider or Custom endpoint. Its access and retention policies apply. To change a Custom endpoint, create a separate profile; its key and history are not reused." }
    ] : [
        { title: "Messages and providers", body: "Messages go directly to the selected provider. Switching providers starts a new chat; existing chats remain with their original provider." }
    ]
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: page.topic === "profile" ? "Custom profile help" : page.topic === "key" ? "API key help" : "Help" }
            Repeater {
                model: page.sections
                delegate: Column {
                    width: parent.width
                    spacing: Theme.paddingMedium
                    SectionHeader { text: modelData.title }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        text: modelData.body
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Sailmate " + Qt.application.version
                textFormat: Text.PlainText
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}