import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property string providerTitle
    property string baseUrl: ""
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
            DialogHeader { title: "Remove API key?"; acceptText: "Remove" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Remove the saved " + providerTitle + " key from this device? Conversation history"
                      + (baseUrl ? " and the custom profile are kept." : " is kept.")
                      + " This does not revoke the key at the provider."
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: baseUrl
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.WrapAnywhere
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}
