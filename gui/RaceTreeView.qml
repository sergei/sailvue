import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtLocation

import sails

Rectangle {
    anchors.fill: parent
    required property var model

    ColumnLayout {
        Layout.leftMargin: 20

        // Chapters list
        TreeView {
            id: racesTreeView
            visible: true

            Layout.fillHeight: true
            Layout.fillWidth: true

            alternatingRows: false
            clip: true
            model: model
            delegate: TreeViewDelegate {}
            selectionModel: ItemSelectionModel {
                onCurrentChanged: function (current, previous) {
                    model.currentChanged(current, previous)
                    win.isRaceSelected = model.isRaceSelected()
                    win.isChapterSelected = model.isChapterSelected()
                    win.selectedName = model.getSelectedName()
                }
                onSelectionChanged: function (current, previous) {
                    model.selectionChanged(current, previous)
                }
            }
        }

        Button {
            id: deleteRaceOrChapterButton
            text: "Delete " + selectedName
            enabled: isRaceSelected || isChapterSelected
            onClicked: {
                model.deleteSelected()
            }
        }

        Button {
            id: addChapterButton
            text: "Add chapter"
            onClicked: {
                model.addChapter()
            }
        }

        Button {
            id: addRaceButton
            text: "Split race"
            onClicked: {
                model.splitRace()
            }
        }
    }

}