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
        delegate: BackgroundItem {
            width: ListView.view.width
            visible: modelData.provider === chat.provider
            height: visible ? Theme.itemSizeLarge : 0
            enabled: !chat.busy && !vault.busy
            onClicked: {
                if (chat.busy || vault.busy) return
                page.openChat(modelData.id)
            }
            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                Label { width: parent.width; text: modelData.title; textFormat: Text.PlainText; truncationMode: TruncationMode.Fade }
                Label { width: parent.width; text: modelData.model; textFormat: Text.PlainText; font.pixelSize: Theme.fontSizeSmall; color: Theme.secondaryColor; truncationMode: TruncationMode.Fade }
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
