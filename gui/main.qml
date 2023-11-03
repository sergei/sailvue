import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtPositioning
import QtLocation
import QtMultimedia
import QtQuick.Controls.Fusion
import sails

ApplicationWindow {
    visible: true

    // --------------------------------------------------
    // Models
    // --------------------------------------------------
    RaceTreeModel {
        id: raceTreeModel
    }


    // --------------------------------------------------
    // Views
    // --------------------------------------------------

    width: 800
    height: 600

    SplitView {
        id: mainSplitView
        anchors.fill: parent
        orientation: Qt.Vertical

        Rectangle {
            id: topPane
            SplitView.minimumHeight: 100
            SplitView.fillHeight: true

            SplitView {
                id: mapVideoSplitView
                anchors.fill: parent
                orientation: Qt.Horizontal

                Rectangle {
                    id: sideBar
                    SplitView.minimumWidth: 200
                    implicitWidth: 320
                    SplitView.fillWidth: false
                    color: "#848282"

                    RaceTreeView {
                        id: raceTreeView
                        model: raceTreeModel
                    }

                }

                Rectangle {
                    id: mapArea
                    SplitView.fillWidth: true
                    SplitView.minimumWidth: 200
                    color: "#5b5a5a"

                    RaceMap {
                        id: raceMap
                        model: raceTreeModel
                    }
                }

                Rectangle {
                    id: videoArea
                    SplitView.fillWidth: false
                    SplitView.minimumWidth: 200
                    implicitWidth: 640
                    color: "#848282"

                    RaceVideo {
                        id: raceVideo
                        model: raceTreeModel
                    }

                }
            }
        }

        Rectangle {
            id: bottomPane
            SplitView.minimumHeight: 100
            implicitHeight: 150
            SplitView.fillHeight: false
            color: "#5b5a5a"
        }
    }


}
