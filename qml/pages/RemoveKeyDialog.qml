import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property string providerTitle
    Column {
        width: parent.width
        spacing: Theme.paddingLarge
        DialogHeader { title: "Remove API key?"; acceptText: "Remove" }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: "Remove the saved " + providerTitle + " key from this device? Conversation history is kept. This does not revoke the key at the provider."
            wrapMode: Text.Wrap
        }
    }
}
