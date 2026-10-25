import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy
    SilicaListView {
        anchors.fill: parent
        model: chat.models
        header: PageHeader { title: "Choose model" }
        delegate: BackgroundItem {
            width: ListView.view.width
            enabled: !chat.busy && !vault.busy
            onClicked: {
                if (chat.busy || vault.busy) return
                chat.setModel(modelData)
                pageStack.pop()
            }
            Label {
                anchors { left: parent.left; right: parent.right; margins: Theme.horizontalPageMargin; verticalCenter: parent.verticalCenter }
                text: modelData
                textFormat: Text.PlainText
                color: modelData === chat.model ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        VerticalScrollDecorator {}
    }
}
