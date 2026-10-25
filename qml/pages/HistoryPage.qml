import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    SilicaListView {
        id: history
        anchors.fill: parent
        model: chat.conversations
        header: Column {
            width: history.width
            spacing: Theme.paddingSmall
            PageHeader { title: "Chat history" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.providerTitle
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.providerEndpoint
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.WrapAnywhere
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingMedium }
        }
        PullDownMenu {
            MenuItem { text: "Change provider"; enabled: !chat.busy && !vault.busy; onClicked: app.showProviders() }
            MenuItem {
                text: "Start new chat"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    if (chat.busy || vault.busy) return
                    chat.newChat()
                    pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
                }
            }
        }
        footer: Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: chat.error
            textFormat: Text.PlainText
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
        delegate: BackgroundItem {
            width: ListView.view.width
            visible: modelData.provider === chat.provider
            height: visible ? Theme.itemSizeLarge : 0
            enabled: !chat.busy && !vault.busy
            onClicked: {
                if (chat.busy || vault.busy) return
                chat.openConversation(modelData.id)
                if (chat.error.length === 0)
                    pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
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
