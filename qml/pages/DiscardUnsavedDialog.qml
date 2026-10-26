import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    allowedOrientations: Orientation.All
    canAccept: chat.unsaved && !chat.busy && !vault.busy
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            DialogHeader { title: "Discard unsaved changes?"; acceptText: "Discard" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "The unsaved messages will be lost and this conversation will close. Previously saved history will not be deleted. This cannot be undone."
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Button {
                id: copyButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Copy conversation"
                enabled: !chat.busy && !vault.busy
                onClicked: {
                    var lines = []
                    for (var index = 0; index < chat.messages.length; ++index) {
                        var message = chat.messages[index]
                        lines.push((message.role === "user" ? "You" : "Sailmate") + ":\n" + message.content)
                    }
                    Clipboard.text = lines.join("\n\n")
                    copyNotice.visible = true
                }
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Copying places the conversation on the system clipboard. It may contain private information."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                id: copyNotice
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: false
                text: "Conversation copied"
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}