import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy

    function filterModels(models, query) {
        var search = query.trim().toLowerCase()
        return models.filter(function(modelId) {
            return modelId.toLowerCase().indexOf(search) !== -1
        })
    }

    Column {
        id: header
        width: parent.width
        PageHeader { title: "Choose model" }
        SearchField {
            id: searchField
            width: parent.width
            placeholderText: "Search models"
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: text.length > 0
            text: chat.error
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    SilicaListView {
        id: modelList
        anchors { top: header.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        clip: true
        model: page.filterModels(chat.models, searchField.text)
        onModelChanged: positionViewAtBeginning()
        delegate: BackgroundItem {
            width: ListView.view.width
            enabled: !chat.busy && !vault.busy
            onClicked: {
                if (chat.busy || vault.busy) return
                chat.setModel(modelData)
                if (chat.model === modelData && !chat.error) pageStack.pop()
            }
            Label {
                anchors { left: parent.left; right: parent.right; margins: Theme.horizontalPageMargin; verticalCenter: parent.verticalCenter }
                text: modelData
                textFormat: Text.PlainText
                color: modelData === chat.model ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }
        ViewPlaceholder {
            enabled: modelList.count === 0
            text: chat.models.length === 0 ? "No models available" : "No matching models"
        }
        VerticalScrollDecorator {}
    }
}
