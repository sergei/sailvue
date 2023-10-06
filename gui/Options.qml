import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import sails
import QtQuick.Controls.Fusion

Dialog {
    id: optionsDialog
    height: 600
    width: 800
    closePolicy : Dialog.NoAutoClose

    title: "Options"
    anchors.centerIn: Overlay.overlay

    standardButtons: Dialog.Ok | Dialog.Cancel

    onAccepted: {
        console.log("OK clicked")
    }

    onRejected: {
        console.log("Cancel clicked")
    }

    ColumnLayout {
        spacing: 2
        anchors.fill: parent

        TabBar {
            id: bar
            anchors.fill: parent
            TabButton {
                text: qsTr("Polars")
            }
            TabButton {
                text: qsTr("PGN List")
            }
            TabButton {
                text: qsTr("Calibration")
            }
        }

        StackLayout {
            currentIndex: bar.currentIndex
            Item {
                id: polarsTab
                RowLayout {
                    spacing: 2
                    Label {
                        id: polarFileLabel
                        text: raceTreeModel.polarPath
                    }
                    Button {
                        text: "Select Polar file"
                        onClicked: polarFileDialog.open()
                    }
                }
            }
            Item {
                id: pgnTab
                PGNSources {
                    id: pgnSources
                }
            }
            Item {
                id: calibrationTab
                GridLayout {
                    columns: 3
                    Label {
                        text: "TWA offset (add this value to TWA)"
                    }
                    TextEdit {
                        id: twa_offset_value
                        text: raceTreeModel.twaOffset
                        color: "white"
                    }
                    Button {
                        text: "Save"
                        onClicked: {
                            raceTreeModel.twaOffset = parseFloat(twa_offset_value.text)
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: polarFileDialog
        visible: false
        title: "Select polar file"
        currentFolder: raceTreeModel.polarPath
        onAccepted: {
            raceTreeModel.polarPath = currentFile
        }
    }
}



