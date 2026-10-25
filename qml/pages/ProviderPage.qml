import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property string pendingProvider: ""
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            PageHeader { title: "Sailmate"; description: "Your AI companion" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Choose your provider"
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }
            Repeater {
                model: [ { providerId: "openai", title: "OpenAI", detail: "API key required. ChatGPT subscriptions do not include API usage." },
                         { providerId: "anthropic", title: "Anthropic · Claude", detail: "An Anthropic API key and API billing are required." } ]
                delegate: BackgroundItem {
                    id: providerItem
                    width: column.width
                    height: info.height + 2 * Theme.paddingLarge
                    enabled: !vault.busy && !chat.busy
                    property bool saved: vault.savedProviders.indexOf(modelData.providerId) !== -1
                    onClicked: {
                        if (vault.busy || chat.busy) return
                        chat.clearError()
                        if (saved) {
                            page.pendingProvider = modelData.providerId
                            vault.loadKey(modelData.providerId)
                        } else {
                            pageStack.push(Qt.resolvedUrl("KeyPage.qml"), {
                                providerId: modelData.providerId,
                                providerTitle: modelData.title
                            })
                        }
                    }
                    Column {
                        id: info
                        x: Theme.horizontalPageMargin
                        y: Theme.paddingLarge
                        width: parent.width - 2 * x - Theme.itemSizeSmall
                        spacing: Theme.paddingSmall
                        Label { text: modelData.title; color: providerItem.highlighted ? Theme.highlightColor : Theme.primaryColor }
                        Label {
                            width: parent.width
                            text: providerItem.saved ? "Key saved · Tap to start a new chat" : modelData.detail
                            wrapMode: Text.Wrap
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.secondaryColor
                        }
                    }
                    IconButton {
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "image://theme/icon-m-delete"
                        visible: providerItem.saved
                        enabled: !vault.busy && !chat.busy
                        onClicked: {
                            if (vault.busy || chat.busy) return
                            var dialog = pageStack.push(Qt.resolvedUrl("RemoveKeyDialog.qml"), { providerTitle: modelData.title })
                            var provider = modelData.providerId
                            dialog.accepted.connect(function() {
                                if (!vault.busy && !chat.busy) vault.removeKey(provider)
                            })
                        }
                    }
                }
            }
            BackgroundItem {
                id: customItem
                width: column.width
                height: customInfo.height + 2 * Theme.paddingLarge
                enabled: !vault.busy && !chat.busy
                onClicked: {
                    if (vault.busy || chat.busy) return
                    chat.clearError()
                    pageStack.push(Qt.resolvedUrl("CustomProvidersPage.qml"))
                }
                Column {
                    id: customInfo
                    x: Theme.horizontalPageMargin
                    y: Theme.paddingLarge
                    width: parent.width - 2 * x
                    spacing: Theme.paddingSmall
                    Label { text: "Custom"; color: customItem.highlighted ? Theme.highlightColor : Theme.primaryColor }
                    Label {
                        width: parent.width
                        text: "Named OpenAI-compatible API profiles. Choose where your messages are sent."
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                }
            }
            BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; running: vault.busy || chat.busy; visible: running; size: BusyIndicatorSize.Medium }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.error || vault.error
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Messages go directly to the selected provider. Switching providers starts a new chat; existing chats remain with their original provider."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        VerticalScrollDecorator {}
    }
    Connections {
        target: vault
        onKeyRemoved: {
            if (provider === "openai" || provider === "anthropic") chat.forgetProvider(provider)
        }
        onKeyLoaded: {
            if (page.status === PageStatus.Active && page.pendingProvider === provider && !chat.busy) {
                page.pendingProvider = ""
                chat.activateProvider(provider, key)
                if (chat.provider === provider && chat.error.length === 0) app.showChat()
            }
        }
    }
}
