import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property string chatTitle
    allowedOrientations: Orientation.All
    canAccept: !chat.busy && !vault.busy && !chat.unsaved && !chat.imageBusy
    backNavigation: !chat.busy && !vault.busy
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            DialogHeader { title: "Delete chat?"; acceptText: "Delete" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chatTitle
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Permanently delete this chat and its saved photos from this device? This cannot be undone. Other chats, profiles and API keys are kept. Data held by the provider is not deleted."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}