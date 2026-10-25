import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: app
    initialPage: Component { ProviderPage {} }
    cover: Component {
        CoverBackground {
            Label {
                anchors.centerIn: parent
                text: "Sailmate"
                color: Theme.highlightColor
            }
        }
    }
    Component { id: providerPage; ProviderPage {} }
    Component { id: historyPage; HistoryPage {} }
    Component { id: chatPage; ChatPage {} }

    function showProviders() {
        pageStack.replaceAbove(null, providerPage)
    }
    function showChat() {
        pageStack.replaceAbove(null, [historyPage, chatPage])
    }
}
