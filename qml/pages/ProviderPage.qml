import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property string pendingProvider: ""

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
                        onClicked: {
                            var dialog = pageStack.push(Qt.resolvedUrl("RemoveKeyDialog.qml"), { providerTitle: modelData.title })
                            var provider = modelData.providerId
                            dialog.accepted.connect(function() {
                                chat.forgetProvider(provider)
                                vault.removeKey(provider)
                            })
                        }
                    }
                }
            }
            BusyIndicator { anchors.horizontalCenter: parent.horizontalCenter; running: vault.busy; visible: running; size: BusyIndicatorSize.Medium }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: vault.error
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
        onKeyLoaded: {
            if (page.pendingProvider === provider) {
                page.pendingProvider = ""
                chat.activateProvider(provider, key)
                app.showChat()
            }
        }
    }
}
