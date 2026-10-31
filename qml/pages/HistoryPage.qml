import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    function openChat(conversation) {
        if (chat.busy || vault.busy || chat.unsaved) return
        if (conversation) chat.openConversation(conversation)
        else chat.newChat()
        if (!chat.error) pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
    }

    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    SilicaListView {
        id: history
        anchors.fill: parent
        model: chat.conversations
        header: PageHeader { title: "Chat history" }
        PullDownMenu {
            MenuItem { text: "Home"; enabled: !chat.busy && !vault.busy; onClicked: app.showProviders() }
            MenuItem {
                text: "Status"
                visible: chat.provider.indexOf("custom-") === 0
                enabled: !chat.busy && !vault.busy
                onClicked: pageStack.push(Qt.resolvedUrl("ProviderStatusPage.qml"), { providerId: chat.provider })
            }
            MenuItem {
                text: "Start new chat"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    if (chat.busy || vault.busy) return
                    page.openChat("")
                }
            }
        }
        footer: Column {
            width: history.width
            spacing: Theme.paddingMedium
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Load older chats"
                visible: chat.hasMoreConversations
                enabled: !chat.busy && !vault.busy
                onClicked: chat.loadMoreConversations()
            }
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
        }
        delegate: ListItem {
            id: chatItem
            width: ListView.view.width
            visible: modelData.provider === chat.provider
            contentHeight: visible ? Theme.itemSizeLarge : 0
            enabled: !chat.busy && !vault.busy && !chat.unsaved && !chat.imageBusy
            menu: ContextMenu {
                MenuItem {
                    text: "Delete chat"
                    enabled: !chat.busy && !vault.busy && !chat.unsaved && !chat.imageBusy
                    onClicked: {
                        var conversationId = modelData.id
                        var providerId = modelData.provider
                        var dialog = pageStack.push(Qt.resolvedUrl("DeleteChatDialog.qml"), { chatTitle: modelData.title })
                        dialog.accepted.connect(function() {
                            if (!vault.busy) chat.deleteConversation(conversationId, providerId)
                        })
                    }
                }
            }
            onClicked: {
                if (chat.busy || vault.busy) return
                page.openChat(modelData.id)
            }
            Column {
                x: Theme.horizontalPageMargin
                width: timeLabel.x - x - Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                Label { width: parent.width; text: modelData.title; textFormat: Text.PlainText; truncationMode: TruncationMode.Fade }
                Label { width: parent.width; text: modelData.model; textFormat: Text.PlainText; font.pixelSize: Theme.fontSizeSmall; color: Theme.secondaryColor; truncationMode: TruncationMode.Fade }
            }
            Label {
                id: timeLabel
                anchors { right: parent.right; rightMargin: Theme.horizontalPageMargin; verticalCenter: parent.verticalCenter }
                width: Theme.fontSizeSmall * 4.5
                text: modelData.lastUsedTime || ""
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignRight
            }
        }
        ViewPlaceholder {
            enabled: chat.conversations.length === 0 && chat.error.length === 0
            text: "No saved chats"
            hintText: "Pull down to start a new chat."
        }
        VerticalScrollDecorator {}
    }
}
