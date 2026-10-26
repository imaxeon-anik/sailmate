import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: "Help" }
            SectionHeader { text: "Messages and providers" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Messages go directly to the selected provider. Switching providers starts a new chat; existing chats remain with their original provider."
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}