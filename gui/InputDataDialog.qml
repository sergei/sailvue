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

        // GOPRO
        Label {
            id: goproFolderLabel
            text: raceTreeModel.goproPath
        }
        Button {
            text: "Select GOPRO folder"
            onClicked: goProFolderDialog.open()
        }

        // Insta 360
        Label {
            text: raceTreeModel.insta360Path
        }
        Button {
            text: "Select Insta360 folder"
            onClicked: insta360FolderDialog.open()
        }

        // Adobe markers
        Label {
            text: raceTreeModel.adobeMarkersPath
        }
        Button {
            text: "Select Adobe markers folder"
            onClicked: adobeMarkersFolderDialog.open()
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
        id: insta360FolderDialog
        visible: false
        title: "Select Insta360 folder"
        currentFolder: raceTreeModel.insta360Path
        onAccepted: {
            raceTreeModel.insta360Path = selectedFolder
        }
    }

    FolderDialog {
        id: adobeMarkersFolderDialog
        visible: false
        title: "Select Adobe markers folder"
        currentFolder: raceTreeModel.adobeMarkersPath
        onAccepted: {
            raceTreeModel.adobeMarkersPath = selectedFolder
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



