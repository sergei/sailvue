import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

Dialog {
    property var raceTreeModel: raceTreeModel

    title: "Specify input data location"
    anchors.centerIn: Overlay.overlay

    standardButtons: Dialog.Ok | Dialog.Cancel

    onAccepted: {
        raceTreeModel.read(ignoreCachedData.checked)
        console.log("OK clicked")
    }

    onRejected: {
        console.log("Cancel clicked")
    }

    // Files Selection
    GridLayout {
        id: selectionGrid
        columns: 1
        visible: true

        Label {
            id: goproFolderLabel
            text: raceTreeModel.goproPath
        }
        Button {
            text: "Select GOPRO folder"
            onClicked: goProFolderDialog.open()
        }

        Label {
            id: nmeaFolderLabel
            text: raceTreeModel.nmeaPath
        }

        Label {
            text: "Instrument logs type"
        }

        ComboBox {
            model: ["YDVR", "Expedition"]
            onActivated: function (idx) {
                if( idx == 0 ) {
                    selectFolder.text = "Select YDVR logs folder"
                    raceTreeModel.logsType = "YDVR"
                } else{
                    selectFolder.text = "Select Expedition Logs folder"
                    raceTreeModel.logsType = "EXPEDITION"
                }
            }
        }

        Button {
            id: selectFolder
            text: "Select YDVR logs folder"
            onClicked: nmeaFolderDialog.open()
        }

        CheckBox {
            id: ignoreCachedData
            checked: false
            text: qsTr("Ignore cached data")
        }
    }

    FolderDialog {
        id: goProFolderDialog
        visible: false
        title: "Select GOPRO folder"
        currentFolder: raceTreeModel.goproPath
        onAccepted: {
            raceTreeModel.goproPath = selectedFolder
        }
    }

    FolderDialog {
        id: nmeaFolderDialog
        visible: false
        title: "Select nmea folder"
        currentFolder: raceTreeModel.nmeaPath
        onAccepted: {
            raceTreeModel.nmeaPath = selectedFolder
        }
    }

}



