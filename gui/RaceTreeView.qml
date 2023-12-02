import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtLocation
import sails
import QtQuick.Controls.Fusion

Rectangle {
    anchors.fill: parent
    color: "#848282"

    required property var raceModel
    property var isRaceSelected: false
    property var isChapterSelected: false
    property var selectedName: ""

    Component.onCompleted: {
        raceModel.setSelectionModel(racesTreeViewSelectionModel)
    }

    // Chapters list
    TreeView {
        id: racesTreeView
        visible: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: deleteRaceOrChapterButton.top
        anchors.margins: 8

        alternatingRows: false
        clip: true
        model: raceModel
        delegate: TreeViewDelegate {}
        selectionModel: ItemSelectionModel {
            id: racesTreeViewSelectionModel
            onCurrentChanged: function (current, previous) {
                racesTreeView.expandToIndex(current)
                model.currentChanged(current, previous)
                isRaceSelected = model.isRaceSelected()
                isChapterSelected = model.isChapterSelected()
                selectedName = model.getSelectedName()
            }
            onSelectionChanged: function (current, previous) {
                model.selectionChanged(current, previous)
            }
        }
    }

    Button {
        id: deleteRaceOrChapterButton
        text: "Delete \"" + selectedName + "\""
        enabled: isRaceSelected || isChapterSelected
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addChapterButton.top
        onClicked: {
            raceModel.deleteSelected()
        }
    }

    Button {
        id: addChapterButton
        text: "Add chapter"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addRaceButton.top
        onClicked: {
            raceModel.addChapter()
        }
    }

    Button {
        id: addRaceButton
        text: "Split race"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        onClicked: {
            raceModel.splitRace()
        }
    }

}