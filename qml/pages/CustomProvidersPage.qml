import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    property string deletingProvider: ""

    SilicaListView {
        id: profiles
        anchors.fill: parent
        model: chat.customProfiles
        header: PageHeader { title: "Custom providers" }
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
        delegate: ListItem {
            id: profileItem
            width: profiles.width
            contentHeight: Math.max(Theme.itemSizeLarge, info.height + 2 * Theme.paddingLarge)
            enabled: !chat.busy && !vault.busy
            property bool saved: vault.savedProviders.indexOf(modelData.id) !== -1
            menu: ContextMenu {
                MenuItem {
                    text: "Remove saved key"
                    enabled: profileItem.saved && !chat.busy && !vault.busy
                    onClicked: {
                        var provider = modelData.id
                        var dialog = pageStack.push(Qt.resolvedUrl("RemoveKeyDialog.qml"), {
                            providerTitle: modelData.name, baseUrl: modelData.baseUrl
                        })
                        dialog.accepted.connect(function() {
                            if (!chat.busy && !vault.busy) vault.removeKey(provider)
                        })
                    }
                }
            }
            onClicked: {
                if (chat.busy || vault.busy) return
                chat.showProviderHistory(modelData.id)
                if (!chat.error) pageStack.push(Qt.resolvedUrl("HistoryPage.qml"))
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
                    text: profileItem.saved ? "Key saved" : "No saved key"
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
                Accessible.name: "Delete custom profile"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    if (chat.busy || vault.busy) return
                    var provider = modelData.id
                    var dialog = pageStack.push(Qt.resolvedUrl("DeleteProfileDialog.qml"), {
                        providerTitle: modelData.name,
                        baseUrl: modelData.baseUrl
                    })
                    dialog.accepted.connect(function() {
                        if (!chat.busy && !vault.busy) {
                            page.deletingProvider = provider
                            vault.removeKey(provider)
                        }
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
            if (provider === page.deletingProvider) {
                page.deletingProvider = ""
                chat.forgetProvider(provider)
                chat.deleteCustomProfile(provider)
            } else if (provider.indexOf("custom-") === 0) {
                chat.forgetProvider(provider)
            }
        }
        onErrorChanged: {
            if (vault.error) page.deletingProvider = ""
        }
    }
}
