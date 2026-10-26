import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property string providerTitle
    property string baseUrl
    allowedOrientations: Orientation.All
    canAccept: !chat.busy && !vault.busy
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            DialogHeader { title: "Delete custom profile?"; acceptText: "Delete" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Permanently delete " + providerTitle + ", its saved API key and all its local chats? This cannot be undone. Other profiles are not affected. This does not revoke the key or delete data held by the service."
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: baseUrl
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}