import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

GridLayout {
    columns: 5
    property alias raceName : race_name.text
    visible: false

    signal changed()
    signal makeEvents()

    Label {
        text: "Race:"
    }

    TextEdit {
        id: race_name
        Layout.columnSpan: 2
        text: "Race name"
        color: "white"
    }

    Button {
        text: "Save race name"
        onClicked: {
            changed()
        }
    }

    Button {
        text: "Make events"
        onClicked: {
            makeEvents()
        }
    }

}
