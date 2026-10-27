import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property string providerId
    readonly property var profile: findProfile(chat.customProfiles, providerId)
    readonly property bool keyRecorded: vault.savedProviders.indexOf(providerId) !== -1
    readonly property string sessionState: describeSession(chat.provider === providerId, chat.busy, chat.connected, profile.model || "")

    function findProfile(profiles, provider) {
        for (var index = 0; index < profiles.length; ++index) {
            if (profiles[index].id === provider) return profiles[index]
        }
        return {}
    }

    function describeSession(active, busy, connected, manualModel) {
        if (!active) return "Not connected"
        if (busy) return "Request in progress"
        if (!connected) return "Not connected"
        return manualModel ? "Manual model configured" : "Models loaded"
    }

    allowedOrientations: Orientation.All
    backNavigation: !chat.busy && !vault.busy

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            PageHeader { title: "Status" }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.profile.name || "Profile unavailable"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.profile.baseUrl || ""
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Repeater {
                model: [
                    { title: "Credential record", value: page.keyRecorded ? "Saved on device (recorded)" : "Not recorded" },
                    { title: "Model configuration", value: page.profile.model ? "Manual model: " + page.profile.model : "Automatic discovery" },
                    { title: "Current session", value: page.sessionState }
                ]
                delegate: Column {
                    width: parent.width
                    spacing: Theme.paddingSmall
                    SectionHeader { text: modelData.title }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        text: modelData.value
                        textFormat: Text.PlainText
                        wrapMode: Text.WrapAnywhere
                        color: Theme.secondaryColor
                    }
                }
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: "Recorded credentials are not checked here. Model availability does not verify chat access or billing."
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Item { width: 1; height: Theme.paddingLarge }
        }
        VerticalScrollDecorator {}
    }
}