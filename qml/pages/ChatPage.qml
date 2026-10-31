import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property bool loadingKey: false
    property string connectingProvider: ""
    readonly property bool modelAvailable: chat.models.indexOf(chat.model) !== -1

    function enterKey() {
        var manualModel = ""
        for (var index = 0; index < chat.customProfiles.length; ++index) {
            if (chat.customProfiles[index].id === chat.provider) {
                manualModel = chat.customProfiles[index].model
                break
            }
        }
        pageStack.push(Qt.resolvedUrl("KeyPage.qml"), {
            providerId: chat.provider, providerTitle: chat.providerTitle,
            baseUrl: chat.providerEndpoint, manualModel: manualModel,
            returnToConversation: true
        })
    }

    function messageEntry(source) {
        return {
            role: String(source.role || ""),
            content: String(source.content || ""),
            status: String(source.status || ""),
            image: String(source.image || ""),
            truncated: !!source.truncated
        }
    }

    function syncMessages() {
        var source = chat.messages
        var follow = messages.atYEnd || source.length > messageModel.count
        // Streaming only changes the last exchange, so skip comparing every earlier row and photo.
        var start = 0
        if (source.length === messageModel.count && source.length > 0
                && messageModel.get(0).content === String(source[0].content || ""))
            start = Math.max(0, source.length - 2)
        while (messageModel.count > source.length)
            messageModel.remove(messageModel.count - 1)
        for (var index = start; index < source.length; ++index) {
            var entry = messageEntry(source[index])
            if (index >= messageModel.count) {
                messageModel.append(entry)
                continue
            }
            var current = messageModel.get(index)
            if (current.role !== entry.role || current.content !== entry.content
                    || current.status !== entry.status || current.image !== entry.image
                    || current.truncated !== entry.truncated)
                messageModel.set(index, entry)
        }
        if (follow) scrollTimer.restart()
    }

    function connectSavedKey() {
        if (chat.busy || vault.busy || chat.unsaved) return
        chat.clearError()
        connectingProvider = chat.provider
        loadingKey = true
        vault.loadKey(connectingProvider)
    }

    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy && !chat.unsaved && !chat.imageBusy
    SilicaListView {
        id: messages
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: composer.top }
        clip: true
        model: ListModel { id: messageModel }
        spacing: Theme.paddingMedium
        header: Column {
            width: messages.width
            spacing: Theme.paddingSmall
            PageHeader { title: "Sailmate" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.providerTitle
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.model || (chat.busy ? "Loading models…" : "Not connected")
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: chat.providerEndpoint
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.WrapAnywhere
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            Item { width: 1; height: Theme.paddingMedium }
        }
        PullDownMenu {
            enabled: !chat.imageBusy
            MenuItem { text: "Home"; enabled: !chat.busy && !vault.busy && !chat.unsaved; onClicked: app.showProviders() }
            MenuItem { text: "Change model"; enabled: !chat.busy && !vault.busy && !chat.unsaved && chat.models.length > 0; onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml")) }
            MenuItem { text: "Update API key"; enabled: !chat.busy && !vault.busy && !chat.unsaved; onClicked: page.enterKey() }
        }
        PushUpMenu {
            enabled: !chat.imageBusy
            MenuItem { text: "Change model"; enabled: !chat.busy && !vault.busy && !chat.unsaved && chat.models.length > 0; onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml")) }
            MenuItem { text: "Home"; enabled: !chat.busy && !vault.busy && !chat.unsaved; onClicked: app.showProviders() }
        }
        delegate: Item {
            id: messageRow
            property bool fromUser: model.role === "user"
            width: messages.width
            height: messageContent.height + 2 * Theme.paddingMedium

            Rectangle {
                id: bubble
                x: messageRow.fromUser
                   ? messageRow.width - width - Theme.horizontalPageMargin
                   : Theme.horizontalPageMargin
                width: (messageRow.width - 2 * Theme.horizontalPageMargin)
                       * (messageRow.fromUser ? 0.82 : 1.0)
                height: messageRow.height
                radius: Theme.paddingSmall
                color: Theme.highlightBackgroundColor
                opacity: messageRow.fromUser ? 0.18 : 0
            }
            Column {
                id: messageContent
                x: bubble.x + Theme.paddingMedium
                y: Theme.paddingMedium
                width: bubble.width - 2 * Theme.paddingMedium
                spacing: Theme.paddingSmall
                Label {
                    width: parent.width
                    text: messageRow.fromUser ? "You" : "Sailmate"
                    horizontalAlignment: messageRow.fromUser ? Text.AlignRight : Text.AlignLeft
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label {
                    width: parent.width
                    text: model.content
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.Wrap
                }
                Image {
                    width: parent.width
                    height: visible ? Theme.itemSizeLarge * 2 : 0
                    visible: model.image.length > 0
                    source: model.image
                    sourceSize.width: 640
                    sourceSize.height: 640
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }
                Label {
                    width: parent.width
                    visible: model.status === "failed"
                    text: "Incomplete · not included in future requests"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                }
                Label {
                    width: parent.width
                    visible: model.truncated
                    text: "Cut off at the length limit"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                }
            }
        }
        footer: Column {
            width: messages.width
            height: visible ? implicitHeight : 0
            visible: chat.messages.length === 0 && !chat.busy
            spacing: Theme.paddingLarge
            Item { width: 1; height: Theme.paddingLarge }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Start a conversation"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Your messages will be sent to " + chat.providerTitle
                      + (chat.providerEndpoint ? " at " + chat.providerEndpoint : "") + ". API charges may apply."
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
        }
        VerticalScrollDecorator {}
    }
    Column {
        id: composer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        spacing: Theme.paddingSmall
        Item {
            width: parent.width
            height: visible ? Theme.itemSizeLarge : 0
            visible: chat.pendingImage.length > 0 || chat.imageBusy
            Image {
                anchors { left: parent.left; leftMargin: Theme.horizontalPageMargin }
                width: Theme.itemSizeLarge
                height: parent.height
                source: chat.pendingImage
                sourceSize.width: 200
                sourceSize.height: 200
                fillMode: Image.PreserveAspectFit
                cache: false
            }
            BusyIndicator {
                anchors.centerIn: parent
                running: chat.imageBusy
                visible: running
            }
            IconButton {
                anchors { right: parent.right; rightMargin: Theme.horizontalPageMargin; verticalCenter: parent.verticalCenter }
                icon.source: "image://theme/icon-m-clear"
                Accessible.name: "Remove photo"
                enabled: !chat.imageBusy && !chat.busy && !vault.busy
                onClicked: chat.removeImage()
            }
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: chat.pendingImage.length > 0
            text: "Requires an image-capable model"
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: chat.connected && !page.modelAvailable && !chat.busy
            text: "This chat's model is unavailable. Choose a replacement before sending."
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Choose model"
            visible: chat.connected && !page.modelAvailable && !chat.busy
            enabled: !vault.busy && !chat.unsaved
            onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml"))
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: chat.unsaved || chat.error.length > 0 || vault.error.length > 0
            text: chat.unsaved ? "Response not saved. Retry saving or discard the unsaved changes." : (chat.error || vault.error)
            textFormat: Text.PlainText
            color: Theme.highlightColor
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
        }
        Item {
            width: parent.width
            height: Math.max(input.height, sendButton.height)
            IconButton {
                id: attachButton
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                icon.source: "image://theme/icon-m-attach"
                Accessible.name: "Attach photo"
                enabled: !chat.busy && !vault.busy && !chat.unsaved && !chat.imageBusy
                onClicked: {
                    input.focus = false
                    pageStack.push(Qt.resolvedUrl("AttachImagePage.qml"))
                }
            }
            TextArea {
                id: input
                anchors.left: attachButton.right
                anchors.right: sendButton.left
                height: Math.min(implicitHeight, page.height * 0.3)
                placeholderText: "Message"
                labelVisible: false
                Accessible.name: "Message"
                enabled: !chat.busy && !vault.busy && !chat.unsaved
            }
            IconButton {
                id: sendButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.bottom: parent.bottom
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                icon.source: chat.busy ? "image://theme/icon-m-cancel" : "image://theme/icon-m-send"
                Accessible.name: chat.busy ? "Cancel response" : "Send message"
                enabled: !chat.imageBusy && !vault.busy && (chat.busy || (!chat.unsaved && chat.connected && page.modelAvailable && input.text.trim().length > 0))
                onClicked: {
                    if (vault.busy) return
                    if (chat.busy) chat.cancel()
                    else {
                        var message = input.text
                        input.focus = false
                        chat.sendMessage(message)
                        if (chat.busy) input.text = ""
                    }
                }
            }
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Connect"
            visible: !chat.connected && !chat.busy && !chat.unsaved
            enabled: !vault.busy && !chat.imageBusy
            onClicked: page.connectSavedKey()
        }
        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: vault.busy
            visible: running
            size: BusyIndicatorSize.Medium
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: chat.unsaved ? "Retry save" : "Retry"
            visible: (chat.unsaved || chat.error.length > 0) && !chat.busy
            enabled: !chat.busy && !vault.busy && !chat.imageBusy && !chat.pendingImage.length
            onClicked: chat.retry()
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Discard unsaved changes"
            visible: chat.unsaved
            enabled: !chat.busy && !vault.busy
            onClicked: {
                var dialog = pageStack.push(Qt.resolvedUrl("DiscardUnsavedDialog.qml"))
                dialog.accepted.connect(function() {
                    if (!chat.busy && !vault.busy) chat.discardUnsavedConversation()
                })
            }
        }
        Item { width: 1; height: Theme.paddingMedium }
    }
    Connections {
        target: chat
        onMessagesChanged: page.syncMessages()
    }
    Component.onCompleted: syncMessages()
    onStatusChanged: if (status === PageStatus.Active) messages.positionViewAtEnd()
    Connections {
        target: vault
        onKeyLoaded: {
            if (page.loadingKey && provider === page.connectingProvider && page.status === PageStatus.Active) {
                page.loadingKey = false
                chat.connectProvider(key)
            }
        }
        onKeyMissing: {
            if (page.loadingKey && provider === page.connectingProvider && page.status === PageStatus.Active) {
                page.loadingKey = false
                page.enterKey()
            }
        }
        onErrorChanged: { if (vault.error) page.loadingKey = false }
    }
    Timer {
        id: scrollTimer
        interval: 50
        onTriggered: if (!messages.dragging && !messages.flicking) messages.positionViewAtEnd()
    }
}
