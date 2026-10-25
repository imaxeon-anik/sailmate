import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    SilicaListView {
        id: messages
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: composer.top }
        clip: true
        model: chat.messages
        spacing: Theme.paddingMedium
        header: Column {
            width: messages.width
            spacing: Theme.paddingSmall
            PageHeader { title: "Sailmate" }
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
                text: chat.model || "Loading models…"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.providerEndpoint
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.WrapAnywhere
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            Item { width: 1; height: Theme.paddingMedium }
        }
        PullDownMenu {
            MenuItem { text: "Change provider"; enabled: !chat.busy && !vault.busy; onClicked: app.showProviders() }
            MenuItem { text: "Change model"; enabled: !chat.busy && !vault.busy && chat.models.length > 0; onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml")) }
            MenuItem { text: "Start new chat"; enabled: !chat.busy && !vault.busy; onClicked: chat.newChat() }
        }
        delegate: Item {
            id: messageRow
            property bool fromUser: modelData.role === "user"
            width: messages.width
            height: messageContent.height + 2 * Theme.paddingMedium

            Rectangle {
                id: bubble
                x: messageRow.fromUser
                   ? messageRow.width - width - Theme.horizontalPageMargin
                   : Theme.horizontalPageMargin
                width: (messageRow.width - 2 * Theme.horizontalPageMargin)
                       * (messageRow.fromUser ? 0.82 : 1.0)
                height: messageRow.height
                radius: Theme.paddingSmall
                color: Theme.highlightBackgroundColor
                opacity: messageRow.fromUser ? 0.18 : 0
            }
            Column {
                id: messageContent
                x: bubble.x + Theme.paddingMedium
                y: Theme.paddingMedium
                width: bubble.width - 2 * Theme.paddingMedium
                spacing: Theme.paddingSmall
                Label {
                    width: parent.width
                    text: messageRow.fromUser ? "You" : "Sailmate"
                    horizontalAlignment: messageRow.fromUser ? Text.AlignRight : Text.AlignLeft
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label {
                    width: parent.width
                    text: modelData.content
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignLeft
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
        }
        footer: Column {
            width: messages.width
            height: visible ? implicitHeight : 0
            visible: chat.messages.length === 0 && !chat.busy
            spacing: Theme.paddingLarge
            Item { width: 1; height: Theme.paddingLarge }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Start a conversation"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Your messages will be sent to " + chat.providerTitle
                      + (chat.providerEndpoint ? " at " + chat.providerEndpoint : "") + ". API charges may apply."
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
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
            textFormat: Text.PlainText
            color: Theme.highlightColor
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
        }
        Item {
            width: parent.width
            height: Math.max(input.height, sendButton.height)
            TextArea {
                id: input
                anchors.left: parent.left
                anchors.right: sendButton.left
                height: Math.min(implicitHeight, page.height * 0.3)
                placeholderText: "Message"
                labelVisible: false
                Accessible.name: "Message"
                enabled: !chat.busy && !vault.busy
            }
            IconButton {
                id: sendButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.bottom: parent.bottom
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                icon.source: chat.busy ? "image://theme/icon-m-cancel" : "image://theme/icon-m-send"
                Accessible.name: chat.busy ? "Cancel response" : "Send message"
                enabled: !vault.busy && (chat.busy || (input.text.trim().length > 0 && chat.model.length > 0))
                onClicked: {
                    if (vault.busy) return
                    if (chat.busy) chat.cancel()
                    else {
                        var message = input.text
                        input.focus = false
                        chat.sendMessage(message)
                        if (chat.busy) input.text = ""
                    }
                }
            }
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Retry"
            visible: chat.error.length > 0 && !chat.busy
            enabled: !chat.busy && !vault.busy
            onClicked: chat.retry()
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
