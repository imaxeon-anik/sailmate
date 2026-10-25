import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    SilicaListView {
        id: messages
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: composer.top }
        clip: true
        model: chat.messages
        spacing: Theme.paddingMedium
        header: PageHeader {
            title: "Sailmate · " + (chat.provider === "openai" ? "OpenAI" : "Claude")
            description: chat.model || "Loading models…"
        }
        PullDownMenu {
            MenuItem { text: "Change provider"; enabled: !chat.busy; onClicked: app.showProviders() }
            MenuItem { text: "Change model"; enabled: !chat.busy && chat.models.length > 0; onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml")) }
            MenuItem { text: "Previous chats"; enabled: !chat.busy; onClicked: pageStack.push(Qt.resolvedUrl("HistoryPage.qml")) }
            MenuItem { text: "Start new chat"; enabled: !chat.busy; onClicked: chat.newChat() }
        }
        delegate: Column {
            x: Theme.horizontalPageMargin
            width: messages.width - 2 * x
            spacing: Theme.paddingSmall
            Label {
                text: modelData.role === "user" ? "You" : "Sailmate"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                width: parent.width
                text: modelData.content
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Label {
                width: parent.width
                visible: modelData.status === "failed"
                text: "Incomplete · not included in future requests"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }
        }
        ViewPlaceholder {
            enabled: chat.messages.length === 0 && !chat.busy
            text: "Start a conversation"
            hintText: "Your messages will be sent to " + (chat.provider === "openai" ? "OpenAI" : "Anthropic") + ". API charges may apply."
        }
        VerticalScrollDecorator {}
    }
    Column {
        id: composer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        spacing: Theme.paddingSmall
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: chat.error.length > 0
            text: chat.error
            color: Theme.highlightColor
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
        }
        TextArea {
            id: input
            width: parent.width
            height: Math.min(implicitHeight, page.height * 0.3)
            placeholderText: "Message"
            label: "Message"
            enabled: !chat.busy
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingMedium
            Button {
                text: chat.busy ? "Cancel" : "Send"
                enabled: chat.busy || (input.text.trim().length > 0 && chat.model.length > 0)
                onClicked: {
                    if (chat.busy) chat.cancel()
                    else {
                        var message = input.text
                        input.focus = false
                        chat.sendMessage(message)
                        if (chat.busy) input.text = ""
                    }
                }
            }
            Button {
                text: "Retry"
                visible: chat.error.length > 0 && !chat.busy
                onClicked: chat.retry()
            }
        }
        Item { width: 1; height: Theme.paddingMedium }
    }
    Connections {
        target: chat
        onMessagesChanged: {
            if (messages.atYEnd || chat.messages.length <= 2)
                scrollTimer.restart()
        }
    }
    Timer { id: scrollTimer; interval: 50; onTriggered: messages.positionViewAtEnd() }
}
