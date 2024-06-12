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
            raceTimeLine.onRaceSelected(startIdx, endIdx)
            raceVideo.seekTo(startIdx)
            raceTreeModel.seekToRacePathIdx(startIdx)
        }

        onRaceUnSelected : function () {
            console.log("onRaceUnSelected")
            raceMap.onRaceUnSelected()
        }

        onChapterSelected : function (uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
            console.log("main: onChapterSelected", uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
            raceMap.onChapterSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
            // Update time after race map, since race map depends on race timeline
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

        onChapterUpdated: function (uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
            console.log("onChapterUpdated: " + chapterName + " " + chapterType + " " + startIdx + " " + endIdx + " " + gunIdx)
            raceTimeLine.onChapterSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
            raceMap.updateChapter(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
        }

        onChapterDeleted : function (uuid) {
            raceTimeLine.onChapterDeleted(uuid)
            raceMap.onChapterDeleted(uuid)
        }

        onProduceStarted: function () {
            fileProgressDialog.open()
        }

        onProduceFinished: function (message) {
            fileProgressDialog.close()
            finishedDialogText.text = message
            finishedDialog.open()
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
                text: qsTr("&Import Adobe Pr markers...")
                onTriggered: importAdobeMarkersFileDialog.open()
                enabled: raceTreeModel.projectName !== "Untitled"
            }

            Action {
                text: qsTr("&Export CSV...")
                onTriggered: statsFileDialog.open()
                enabled: raceTreeModel.projectName !== "Untitled"
            }

            Action {
                text: qsTr("&Export GPX...")
                onTriggered: gpxFileDialog.open()
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
            title: qsTr("&Edit")
            delegate: MenuItem {
                id: control

                contentItem: Item {
                    anchors.centerIn: parent

                    Text {
                        text: control.text
                        anchors.left: parent.left
                        color: "white"
                    }

                    Text {
                        text: control.action.shortcut
                        anchors.right: parent.right
                        color: "white"
                    }
                }
            }

            Action {
                text: qsTr("Chapter In -10 sec")
                shortcut: "a"
                onTriggered: moveChapterStartIdx(-10000)
            }
            Action {
                text: qsTr("Chapter In -1 sec")
                shortcut: "z"
                onTriggered: moveChapterStartIdx(-1000)
            }
            Action {
                text: qsTr("Chapter In +1 sec")
                shortcut: "x"
                onTriggered: moveChapterStartIdx(1000)
            }
            Action {
                text: qsTr("Chapter In +10 sec")
                shortcut: "s"
                onTriggered: moveChapterStartIdx(10000)
            }

            MenuSeparator {
                padding: 0
                topPadding: 12
                bottomPadding: 12
                contentItem: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 1
                    color: "#1E000000"
                }
            }

            Action {
                text: qsTr("Chapter Out -10 sec")
                shortcut: ";"
                onTriggered: moveChapterEndIdx(-10000)
            }
            Action {
                text: qsTr("Chapter Out -1 sec")
                shortcut: "."
                onTriggered: moveChapterEndIdx(-1000)
            }
            Action {
                text: qsTr("Chapter Out +1 sec")
                shortcut: "/"
                onTriggered: moveChapterEndIdx(1000)
            }
            Action {
                text: qsTr("Chapter Out +10 sec")
                shortcut: "'"
                onTriggered: moveChapterEndIdx(10000)
            }

            MenuSeparator {
                padding: 0
                topPadding: 12
                bottomPadding: 12
                contentItem: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 1
                    color: "#1E000000"
                }
            }

            Action {
                text: qsTr("Chapter Gun -10 sec")
                shortcut: "d"
                onTriggered: moveChapterGunIdx(-10000)
            }
            Action {
                text: qsTr("Chapter Gun -1 sec")
                shortcut: "c"
                onTriggered: moveChapterGunIdx(-1000)
            }
            Action {
                text: qsTr("Chapter Gun +1 sec")
                shortcut: "f"
                onTriggered: moveChapterGunIdx(1000)
            }
            Action {
                text: qsTr("Chapter Gun +10 sec")
                shortcut: "v"
                onTriggered: moveChapterGunIdx(10000)
            }

        }
        Menu {
            title: qsTr("&Tools")
            Action {
                text: qsTr("&Options...")
                onTriggered: options.open()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About sailvue...")
                onTriggered: aboutDialog.open()
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
        id: importAdobeMarkersFileDialog
        visible: false
        title: "Import Adone Premiere Markers"
        onAccepted: {
            raceTreeModel.importAdobeMarkers(selectedFolder)
        }
    }

    FileDialog {
        id: gpxFileDialog
        visible: false
        title: "Export GPX file"
        fileMode: FileDialog.SaveFile
        nameFilters: ["GPX files (*.gpx)"]
        onAccepted: {
            raceTreeModel.exportGpx(currentFile)
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

    Dialog {
        id: finishedDialog
        anchors.centerIn: Overlay.overlay
        title: "Done"
        modal: true
        contentItem: ColumnLayout {
            Text {
                id: finishedDialogText
                Layout.alignment: Qt.AlignCenter
                text: "Done"
                color: "white"
            }
        }
        standardButtons: Dialog.Ok
        onAccepted: {
            close()
        }
    }

    Options {
        id: options
    }

    Dialog {
        id: aboutDialog
        visible: false
        title: qsTr("About")

        Label {
            text: "GIT hash: " + raceTreeModel.gitHash
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
                    SplitView.minimumWidth: 320
                    implicitWidth: 480
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

                onChapterStartIdxChanged: {
                    console.log('On start change', chapterStartIdx)
                    let idx = Math.round(chapterStartIdx)
                    broadcastChapterStartIdx(idx)
                }

                onChapterEndIdxChanged: {
                    let idx = Math.round(chapterEndIdx)
                    broadcastChapterEndIdx(idx)
                }

                onChapterGunIdxChanged: {
                    let idx = Math.round(chapterGunIdx)
                    broadcastChapterGunIdx(idx)
                }
            }
        }
    }

    function moveChapterStartIdx(ms) {
        raceTimeLine.chapterStartIdx = raceTreeModel.moveIdxByMs(raceTimeLine.chapterStartIdx, ms)
    }
    function moveChapterEndIdx(ms) {
        raceTimeLine.chapterEndIdx = raceTreeModel.moveIdxByMs(raceTimeLine.chapterEndIdx, ms)
    }
    function moveChapterGunIdx(ms) {
        raceTimeLine.chapterGunIdx = raceTreeModel.moveIdxByMs(raceTimeLine.chapterGunIdx, ms)
    }

    function broadcastChapterStartIdx(idx) {
        raceMap.onSelectedChapterStartIdxChanged(idx)
        raceTreeModel.seekToRacePathIdx(idx)
        raceTreeModel.updateChapterStartIdx(idx)
        raceVideo.seekTo(idx)
    }

    function broadcastChapterEndIdx(idx) {
        raceMap.onSelectedChapterEndIdxChanged(idx)
        raceTreeModel.seekToRacePathIdx(idx)
        raceTreeModel.updateChapterEndIdx(idx)
        raceVideo.seekTo(idx)
    }
    function broadcastChapterGunIdx(idx) {
        raceMap.onSelectedChapterGunIdxChanged(idx)
        raceTreeModel.seekToRacePathIdx(idx)
        raceTreeModel.updateChapterGunIdx(idx)
        raceVideo.seekTo(idx)
    }




}
