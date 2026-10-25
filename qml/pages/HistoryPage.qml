import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All
    SilicaListView {
        anchors.fill: parent
        model: chat.conversations
        header: PageHeader { title: "Chat history"; description: chat.provider === "openai" ? "OpenAI" : "Anthropic" }
        PullDownMenu {
            MenuItem { text: "Change provider"; enabled: !chat.busy; onClicked: app.showProviders() }
            MenuItem {
                text: "Start new chat"
                enabled: !chat.busy
                onClicked: {
                    chat.newChat()
                    pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
                }
            }
        }
        footer: Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: chat.error
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
        delegate: BackgroundItem {
            width: ListView.view.width
            visible: modelData.provider === chat.provider
            height: visible ? Theme.itemSizeLarge : 0
            enabled: !chat.busy
            onClicked: {
                chat.openConversation(modelData.id)
                if (chat.error.length === 0)
                    pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
            }
            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                Label { width: parent.width; text: modelData.title; truncationMode: TruncationMode.Fade }
                Label { width: parent.width; text: modelData.model; font.pixelSize: Theme.fontSizeSmall; color: Theme.secondaryColor; truncationMode: TruncationMode.Fade }
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
