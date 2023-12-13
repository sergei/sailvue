import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtLocation
import sails
import QtQuick.Controls.Fusion
import Qt.labs.qmlmodels

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


    DelegateChooser {
        id: chooser
        DelegateChoice { column: 0
            delegate: TextField {
                x: control.contentItem.x
                y: (parent.height - height) / 2
                width: control.contentItem.width
                text: display
                color: "green"
                TableView.onCommit: display = text
            }
        }
        DelegateChoice { column: 1
            delegate: TextField {
                x: control.contentItem.x
                y: (parent.height - height) / 2
                width: control.contentItem.width
                text: display
                color: "red"
                TableView.onCommit: display = text
            }
        }
    }

    // Chapters list
    TreeView {
        id: racesTreeView
        visible: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addChapterButton.top
        anchors.margins: 8

        alternatingRows: false
        clip: false
        model: raceModel
        delegate: DelegateChooser {
            DelegateChoice {
                column: 0
                delegate: Item {
                    id: treeDelegate

                    implicitWidth: padding + label.x + label.implicitWidth + padding
                    implicitHeight: label.implicitHeight * 1.5

                    readonly property real indent: 20
                    readonly property real padding: 5

                    // Assigned to by TreeView:
                    required property TreeView treeView
                    required property bool isTreeNode
                    required property bool expanded
                    required property int hasChildren
                    required property int depth

                    property var highlighted: row === treeView.currentRow

                    TapHandler {
                        onTapped: treeView.toggleExpanded(row)
                    }

                    Text {
                        id: indicator
                        visible: treeDelegate.isTreeNode && treeDelegate.hasChildren
                        x: padding + (treeDelegate.depth * treeDelegate.indent)
                        anchors.verticalCenter: label.verticalCenter
                        text: "▸"
                        rotation: treeDelegate.expanded ? 90 : 0
                    }

                    Text {
                        id: label
                        x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
                        width: treeDelegate.width - treeDelegate.padding - x
                        clip: true
                        text: model.display
                        color: treeDelegate.highlighted ? "white" : "black"
                    }

                    TableView.editDelegate: FocusScope {
                        width: parent.width
                        height: parent.height

                        TextField {
                            id: textField
                            x: label.x
                            y: (parent.height - height) / 2
                            width: label.width
                            text: racesTreeView.model.data(racesTreeView.index(row, column), Qt.DisplayRole)
                            focus: true
                        }

                        TableView.onCommit: {
                            let index = racesTreeView.index(row, column)
                            racesTreeView.model.setData(index, textField.text, Qt.EditRole)
                        }
                        Component.onCompleted: textField.selectAll()
                    }
                }
            }
            DelegateChoice {
                column: 1
                delegate: ComboBox {
                    id: chapter_type
                    visible: racesTreeView.model.data(racesTreeView.index(row, column), Qt.DisplayRole) !== -1
                    currentIndex: racesTreeView.model.data(racesTreeView.index(row, column), Qt.DisplayRole)
                    model: ["Tack/Gybe", "Performance", "Start", "Mark rounding"]
                    onActivated: function (ord) {
                        racesTreeView.model.setData(racesTreeView.index(row, column), ord)
                    }
                }
            }
            DelegateChoice {
                column: 2
                delegate: Button {
                    implicitWidth: 32
                    implicitHeight: 32
                    Image {
                        anchors.fill: parent
                        source: "qrc:/images/icons8-delete-button-50.png"
                    }
                    onClicked: {
                        racesTreeView.model.setData(racesTreeView.index(row, column), "delete")
                    }
                }
            }
        }

        editTriggers: TableView.DoubleTapped

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
        id: addChapterButton
        text: "Add chapter"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: detectManeuvers.top
        onClicked: {
            raceModel.addChapter()
        }
    }

    Button {
        id: detectManeuvers
        text: "Detect maneuvers"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: makeAnalytics.top
        onClicked: {
            raceModel.detectManeuvers()
        }
    }

    Button {
        id: makeAnalytics
        text: "Make analytics"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addRaceButton.top
        onClicked: {
            raceModel.makeAnalytics()
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