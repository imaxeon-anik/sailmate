import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    SilicaListView {
        anchors.fill: parent
        model: chat.models
        header: PageHeader { title: "Choose model" }
        delegate: BackgroundItem {
            width: ListView.view.width
            enabled: !chat.busy
            onClicked: { chat.setModel(modelData); pageStack.pop() }
            Label {
                anchors { left: parent.left; right: parent.right; margins: Theme.horizontalPageMargin; verticalCenter: parent.verticalCenter }
                text: modelData
                color: modelData === chat.model ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        VerticalScrollDecorator {}
    }
}
