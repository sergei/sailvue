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
    id: app

    visible: true
    title: raceTreeModel.projectName + (raceTreeModel.isDirty ? "*" : "")
    width: videoArea.implicitWidth + sideBar.implicitWidth + mapArea.implicitWidth
    height: 800

    property ApplicationWindow appWindow : app


    // --------------------------------------------------
    // Functions
    // --------------------------------------------------


    // --------------------------------------------------
    // Models
    // --------------------------------------------------
    RaceTreeModel {
        id: raceTreeModel

        // Reading data from files
        onProgressStatus: function (state, progress) {
            progressBar.value = progress
            progressText.text = state
        }

        onLoadStarted: function () {
            fileProgressDialog.open()
        }

        // All data read
        onLoadFinished: function () {
            fileProgressDialog.close()
            raceTreeModel.seekToRacePathIdx(0)
            raceVideo.seekTo(0)
        }

        onFullPathReady: function (raceGeoPath) {
            raceMap.onFullPathReady(raceGeoPath)

            raceTimeLine.raceLength = raceGeoPath.path.length
            raceTimeLine.raceStartidx = 0
        }

        onRacePathIdxChanged: function (racePathIdx) {
            // Set the position marker on the map
            raceMap.onRacePathIdxChanged(racePathIdx)

            // Update timeline
            raceTimeLine.onRacePathIdxChanged(racePathIdx)
        }

        onRaceSelected : function (raceName, startIdx, endIdx) {
            console.log("onRaceSelected", raceName, startIdx, endIdx)
            raceMap.onRaceSelected(startIdx, endIdx)
            raceTimeLine.raceName = raceName
            raceTimeLine.onRaceSelected(startIdx, endIdx)
            raceVideo.seekTo(startIdx)
            raceTreeModel.seekToRacePathIdx(startIdx)
        }


        onChapterSelected : function (uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
            raceTimeLine.onChapterSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)

            if ( chapterType === ChapterTypes.START || chapterType === ChapterTypes.MARK_ROUNDING) {
                raceVideo.seekTo(gunIdx)
                raceTreeModel.seekToRacePathIdx(gunIdx)
            } else{
                raceVideo.seekTo(startIdx)
                raceTreeModel.seekToRacePathIdx(startIdx)
            }
        }

        onChapterUnSelected : function (uuid) {
            raceTimeLine.onChapterUnSelected(uuid)
        }

        onChapterAdded : function (uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
            raceMap.onChapterAdded(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
        }

        onChapterDeleted : function (uuid) {
            raceTimeLine.onChapterDeleted(uuid)
            raceMap.onChapterDeleted(uuid)
        }

        onProduceStarted: function () {
            fileProgressDialog.open()
        }

        onProduceFinished: function () {
            fileProgressDialog.close()
        }

    }


    // --------------------------------------------------
    // Views
    // --------------------------------------------------

    // --------------------------------------------------
    // Menus
    // --------------------------------------------------

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&New Project...")
                onTriggered: inputDataDialog.open()
            }
            Action {
                text: qsTr("&Open project...")
                onTriggered: projectOpenDialog.open()
            }
            Action {
                text: qsTr("&Save project")
                onTriggered: raceTreeModel.save()
                enabled: raceTreeModel.isDirty && raceTreeModel.projectName !== "Untitled"
            }
            Action {
                text: qsTr("Save project &As...")
                onTriggered: projectSaveAsFileDialog.open()
                enabled: raceTreeModel.isDirty
            }

            Action {
                text: qsTr("&Export Stats...")
                onTriggered: statsFileDialog.open()
                enabled: raceTreeModel.projectName !== "Untitled"
            }

            Action {
                text: qsTr("&Produce...")
                onTriggered: produceFolderDialog.open()
                enabled: raceTreeModel.projectName !== "Untitled"

            }
            MenuSeparator { }
            Action {
                text: qsTr("&Quit")
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("&Tools")
            Action {
                text: qsTr("&Options...")
                onTriggered: options.open()
            }
        }
    }

    // --------------------------------------------------
    // File dialogs
    // --------------------------------------------------


    InputDataDialog {
        id: inputDataDialog
        raceTreeModel: raceTreeModel
    }

    FileDialog {
        id: projectOpenDialog
        visible: false
        title: "Open project file"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Project files (*.json)"]
        onAccepted: {
            raceTreeModel.load(currentFile)
        }
    }

    FileDialog {
        id: projectSaveAsFileDialog
        visible: false
        title: "Save project file"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Project files (*.json)"]
        onAccepted: {
            raceTreeModel.saveAs(currentFile)
        }
    }

    FileDialog {
        id: statsFileDialog
        visible: false
        title: "Export stats CSV file"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Stat files (*.csv)"]
        onAccepted: {
            raceTreeModel.exportStats(raceTreeModel.polarPath, currentFile)
        }
    }

    FolderDialog {
        id: produceFolderDialog
        visible: false
        title: "Select folder for produced video"
        currentFolder: "file:///tmp"
        onAccepted: {
            raceTreeModel.produce(selectedFolder, raceTreeModel.polarPath)
        }
    }

    // --------------------------------------------------
    // Progress dialogs
    // --------------------------------------------------


    Dialog {
        id: fileProgressDialog
        anchors.centerIn: Overlay.overlay
        title: "Processing data"
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
            raceTreeModel.stopDataProcessing()
        }
    }


    // --------------------------------------------------
    // Widgets
    // --------------------------------------------------
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
                        raceModel: raceTreeModel
                    }

                }

                Rectangle {
                    id: mapArea
                    SplitView.fillWidth: true
                    SplitView.minimumWidth: 200
                    implicitWidth: 640
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
                        raceModel: raceTreeModel
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

            RaceTimeLine {
                id: raceTimeLine
                model: raceTreeModel
                player: raceVideo

                onChapterChanged: function () {
                    console.log("Chapter changed: " + chapterName + " " + chapterType + " " + chapterStartIdx + " " + chapterEndIdx + " " + chapterGunIdx)
                    raceTreeModel.updateChapter(chapterUuid, chapterName, chapterType, chapterStartIdx, chapterEndIdx, chapterGunIdx)
                    raceMap.updateChapter(chapterUuid, chapterName, chapterType, chapterStartIdx, chapterGunIdx, chapterGunIdx)
                }

                onChapterStartIdxChanged: {
                    let idx = Math.round(chapterStartIdx)
                    raceMap.onSelectedChapterStartIdxChanged(idx)
                    raceTreeModel.seekToRacePathIdx(idx)
                }

                onChapterEndIdxChanged: {
                    let idx = Math.round(chapterEndIdx)
                    raceMap.onSelectedChapterEndIdxChanged(idx)
                    raceTreeModel.seekToRacePathIdx(idx)
                }

                onChapterGunIdxChanged: {
                    let idx = Math.round(chapterGunIdx)
                    raceMap.onSelectedChapterGunIdxChanged(idx)
                    raceTreeModel.seekToRacePathIdx(idx)
                }


            }

        }
    }


}
