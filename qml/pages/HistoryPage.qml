import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    SilicaListView {
        anchors.fill: parent
        model: chat.conversations
        header: PageHeader { title: "Previous chats"; description: chat.provider === "openai" ? "OpenAI" : "Anthropic" }
        delegate: BackgroundItem {
            width: ListView.view.width
            visible: modelData.provider === chat.provider
            height: visible ? Theme.itemSizeLarge : 0
            enabled: !chat.busy
            onClicked: { chat.openConversation(modelData.id); pageStack.pop() }
            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                Label { width: parent.width; text: modelData.title; truncationMode: TruncationMode.Fade }
                Label { width: parent.width; text: modelData.model; font.pixelSize: Theme.fontSizeSmall; color: Theme.secondaryColor; truncationMode: TruncationMode.Fade }
            }
        }
        ViewPlaceholder { enabled: chat.conversations.length === 0; text: "No saved chats" }
        VerticalScrollDecorator {}
    }
}
