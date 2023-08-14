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

    Label {
        text: "Race:"
    }

    TextEdit {
        id: race_name
        Layout.columnSpan: 3
        text: "Race name"
        color: "white"
    }

    Button {
        text: "Save race name"
        onClicked: {
            changed()
        }
    }

}
