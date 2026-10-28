import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

ImagePickerPage {
    allowedOrientations: Orientation.All
    onSelectedContentPropertiesChanged: {
        if (selectedContentProperties.filePath) {
            chat.attachImage(selectedContentProperties.filePath)
            pageStack.pop()
        }
    }
}