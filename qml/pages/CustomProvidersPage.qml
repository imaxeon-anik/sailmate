import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    property string pendingProvider: ""

    SilicaListView {
        id: profiles
        anchors.fill: parent
        model: chat.customProfiles
        header: Column {
            width: profiles.width
            spacing: Theme.paddingLarge
            PageHeader { title: "Custom providers" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Use only endpoints approved by your organisation. Your API key and conversation are sent to the chosen URL. Profiles cannot be edited; create a separate profile to change an endpoint."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingSmall }
        }
        PullDownMenu {
            MenuItem {
                text: "Add profile"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    if (chat.busy || vault.busy) return
                    chat.clearError()
                    pageStack.push(Qt.resolvedUrl("CustomProfilePage.qml"))
                }
            }
        }
        delegate: BackgroundItem {
            id: profileItem
            width: profiles.width
            height: Math.max(Theme.itemSizeLarge, info.height + 2 * Theme.paddingLarge)
            enabled: !chat.busy && !vault.busy
            property bool saved: vault.savedProviders.indexOf(modelData.id) !== -1
            onClicked: {
                if (chat.busy || vault.busy) return
                chat.clearError()
                if (saved) {
                    page.pendingProvider = modelData.id
                    vault.loadKey(modelData.id)
                } else {
                    pageStack.push(Qt.resolvedUrl("KeyPage.qml"), {
                        providerId: modelData.id,
                        providerTitle: modelData.name,
                        baseUrl: modelData.baseUrl,
                        manualModel: modelData.model
                    })
                }
            }
            Column {
                id: info
                x: Theme.horizontalPageMargin
                y: Theme.paddingLarge
                width: parent.width - 2 * x - Theme.itemSizeSmall - Theme.paddingSmall
                spacing: Theme.paddingSmall
                Label {
                    width: parent.width
                    text: modelData.name
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    color: profileItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: modelData.baseUrl
                    textFormat: Text.PlainText
                    wrapMode: Text.WrapAnywhere
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label {
                    width: parent.width
                    text: modelData.model ? "Manual model: " + modelData.model : "Models discovered when connecting"
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label {
                    width: parent.width
                    text: profileItem.saved ? "Key saved · Tap to start a new chat" : "No saved key · Tap to set up"
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
            IconButton {
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-delete"
                Accessible.name: "Remove saved key"
                visible: profileItem.saved
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    if (chat.busy || vault.busy) return
                    var provider = modelData.id
                    var dialog = pageStack.push(Qt.resolvedUrl("RemoveKeyDialog.qml"), {
                        providerTitle: modelData.name,
                        baseUrl: modelData.baseUrl
                    })
                    dialog.accepted.connect(function() {
                        if (!chat.busy && !vault.busy) vault.removeKey(provider)
                    })
                }
            }
        }
        footer: Column {
            width: profiles.width
            spacing: Theme.paddingLarge
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: chat.busy || vault.busy
                visible: running
                size: BusyIndicatorSize.Medium
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
            Item { width: 1; height: Theme.paddingLarge }
        }
        ViewPlaceholder {
            enabled: chat.customProfiles.length === 0 && !chat.busy && !vault.busy && !chat.error && !vault.error
            text: "No custom profiles"
            hintText: "Pull down to add a named OpenAI-compatible endpoint."
        }
        VerticalScrollDecorator {}
    }
    Connections {
        target: vault
        onKeyRemoved: {
            if (provider !== "openai" && provider !== "anthropic") chat.forgetProvider(provider)
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
