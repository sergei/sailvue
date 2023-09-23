import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import sails
import QtQuick.Controls.Fusion

Item {
    id: dialog

    Component.onCompleted: {
        okButton.enabled = false;
    }

    PgnSrcTreeModel {
        id: pgnSrcTreeModel
        // Reading data from files
        onProgressStatus: function (state, progress) {
            progressBar.value = progress
            progressText.text = state
        }

        // All data read
        onDataReadComplete: function () {
            progressDialog.close()
        }

        onIsDirtyChanged: function () {
            okButton.enabled = pgnSrcTreeModel.isDirty;
        }
    }

    anchors.centerIn: Overlay.overlay

    // Files Selection
    GridLayout {
        id: selectionGrid
        columns: 1
        visible: true

        Button {
            text: "Generate new PGN sources CSV file"
            onClicked: nmeaFolderDialog.open()
        }

        Label {
            id: pgnSrcLabel
            text: pgnSrcTreeModel.pgnSrcCsvPath
        }

        Button {
            text: "Load PGN sources CSV file"
            onClicked: loadPgnScrsCsvFileDialog.open()
        }

        TreeView {
            id: pgnsTreeView
            model: pgnSrcTreeModel
            visible: true

            width: 600
            height: 400
            Layout.fillHeight: true
            Layout.fillWidth: true

            alternatingRows: false
            clip: true
            delegate: TreeViewDelegate {}

            selectionModel: ItemSelectionModel {
                onCurrentChanged: function (current, previous) {
                    pgnSrcTreeModel.currentChanged(current, previous)
                }
            }
        }

        Button {
            id: okButton
            text: "Save PGN sources CSV file"
            onClicked: pgnSrcTreeModel.saveData()
        }

    }


    FolderDialog {
        id: nmeaFolderDialog
        visible: false
        title: "Select nmea folder"
        onAccepted: {
            pgnScrsCsvFileDialog.open()
        }
    }

    FileDialog {
        id: pgnScrsCsvFileDialog
        visible: false
        title: "PGN sources CSV file"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PGN sources files (*.csv)"]
        onAccepted: {
            progressDialog.open()
            pgnSrcTreeModel.readData(nmeaFolderDialog.selectedFolder, pgnScrsCsvFileDialog.selectedFile)
        }
    }

    FileDialog {
        id: loadPgnScrsCsvFileDialog
        visible: false
        title: "PGN sources CSV file"
        fileMode: FileDialog.OpenFile
        nameFilters: ["PGN sources files (*.csv)"]
        onAccepted: {
            pgnSrcTreeModel.loadData(selectedFile)
        }
    }

    Dialog {
        id: progressDialog
        anchors.centerIn: Overlay.overlay
        title: "Processing NMEA sources"
        modal: true
        closePolicy : Dialog.NoAutoClose
        contentItem: ColumnLayout {
            Text {
                id: progressText
                Layout.alignment: Qt.AlignCenter
                text: "Processing data..."
                color: "white"
            }
            ProgressBar {
                id: progressBar
                Layout.alignment: Qt.AlignLeft | Qt.AlignRight
                anchors.margins: 20
                from: 0
                to: 100
                value: 50
            }
        }
        standardButtons: Dialog.Cancel
        onRejected: {
            console.log("Stopping processing")
            pgnSrcTreeModel.stop()
        }
    }

}



