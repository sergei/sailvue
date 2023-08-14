import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtPositioning
import QtLocation
import QtMultimedia
import sails
import QtQuick.Controls.Fusion

ApplicationWindow {

    id: win
    visible: true
    title: raceTreeModel.projectName + (raceTreeModel.isDirty ? "*" : "")

    width: mainGrid.implicitWidth + 40
    height: mainGrid.implicitHeight + 40

    property var fullMapPath  // Path for entire project
    property var chapterMapElements: {'aa': 'bb'}  // Initialize it with some nonsense, otherwise we get an assignment error
    property int raceLength: 0
    property int raceStartidx: 0

    property var isRaceSelected: false
    property var isChapterSelected: false
    property var selectedName: ""

    property var currentClipUrl: ""

    property int isVideoLoaded: 0
    property int isVideoPlaying: 0

    Component.onCompleted: {
        mediaplayer.play()
    }

    MapPolyline {
        id: racePathLine
        line.width: 3
        line.color: "red"
    }

    MapQuickItem {
        id: boatMarker
        anchorPoint.x: boatImage.width / 2
        anchorPoint.y: boatImage.height / 2

        sourceItem: Image {
            id: boatImage
            width: 24
            height: 24
            source: "qrc:/images/mapmarker.png"
        }
    }

    MapItemGroup {
        id: selectedChapterMarkers
        visible: false

            MapQuickItem {
                id: selectedChapterStartMarker
                anchorPoint.x: startImage.width / 2
                anchorPoint.y: startImage.height / 2
                coordinate: QtPositioning.coordinate(0, 0)

                sourceItem: Rectangle {
                    id: startImage
                    width: 18
                    height: 36
                    color: "red"
                    radius: 9
                }
            }

            MapQuickItem {
                id: selectedChapterEndMarker
                anchorPoint.x: endImage.width / 2
                anchorPoint.y: endImage.height / 2
                coordinate: QtPositioning.coordinate(0, 0)

                sourceItem: Rectangle {
                    id: endImage
                    width: 18
                    height: 36
                    color: "yellow"
                    radius: 9
                }
            }

    }

    function hideChapterEditor(uuid) {
        console.log("Hiding chapter editor")
        raceEditor.visible = false
        chapterEditor.visible = false
        selectedChapterMarkers.visible = false
    }

    // C++ class defined in RaceTreeModel.h
    RaceTreeModel {
        id: raceTreeModel

        // Reading data from files
        onProgressStatus: {
            progressBar.value = progress
            progressText.text = state
        }

        // All data read
        onLoadFinished: function () {
            fileProgressDialog.close()
            raceTreeModel.seekToRacePathIdx(0)
            mediaplayer.seekTo(0)
        }

        onFullPathReady: function (raceGeoPath) {
            win.fullMapPath = raceGeoPath.path

            racePathLine.path = win.fullMapPath

            mapView.map.addMapItem(racePathLine)
            mapView.map.addMapItem(boatMarker)
            mapView.map.addMapItemGroup(selectedChapterMarkers)
            mapView.map.center = raceGeoPath.path[0]
            boatMarker.coordinate = win.fullMapPath[0]

            win.raceLength = racePathLine.path.length
            win.raceStartidx = 0
        }

        onRacePathIdxChanged: function (racePathIdx) {
            // Set the position marker on the map
            boatMarker.coordinate = win.fullMapPath[racePathIdx]

            // Update chapter editor
            chapterEditor.gunIdx = racePathIdx

            // Update time label
            currentTimeLabel.text = raceTreeModel.getTimeString(racePathIdx)
        }

        onRaceSelected : function (raceName, startIdx, endIdx) {
            console.log("onRaceSelected", raceName, startIdx, endIdx)
            raceEditor.visible = true
            raceEditor.raceName = raceName
            racePathLine.path = win.fullMapPath.slice(startIdx, endIdx)

            win.raceLength = racePathLine.path.length
            win.raceStartidx = startIdx
            progressSlider.value = 0

            // Clear all chapter map elements
            for(let uuid in win.chapterMapElements) {
                if ( uuid === "aa")  continue  // FIXME need to rid of stupid initializer in the property declaration
                console.log("Removing Chapter map element uuid: " + uuid)
                const chapterMapElement = win.chapterMapElements[uuid]
                console.log("Removing chapter map element: " + chapterMapElement)
                mapView.map.removeMapItemGroup(chapterMapElement)
                win.chapterMapElements[uuid].destroy()
                delete win.chapterMapElements[uuid]
            }

            mediaplayer.seekTo(startIdx)
            raceTreeModel.seekToRacePathIdx(startIdx)
        }


        onChapterSelected : function (uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
            chapterEditor.visible = true
            console.log("Visble: " + chapterEditor.visible)
            chapterEditor.setSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)

            selectedChapterMarkers.visible = true
            selectedChapterStartMarker.coordinate = win.fullMapPath[startIdx]
            selectedChapterEndMarker.coordinate = win.fullMapPath[endIdx]

            mediaplayer.seekTo(startIdx)
            raceTreeModel.seekToRacePathIdx(startIdx)
        }

        onChapterUnSelected : function (uuid) {
            console.log("onChapterUnSelected")
            hideChapterEditor(uuid)
        }

        onChapterAdded : function (uuid, chapterName, chapterType, startIdx, endIdx) {
            let co = Qt.createComponent('ChapterMapElement.qml')
            if (co.status === Component.Ready) {
                let chapterMapElement = co.createObject(win)
                chapterMapElement.path = win.fullMapPath.slice(startIdx, endIdx)
                win.chapterMapElements[uuid] = chapterMapElement
                mapView.map.addMapItemGroup(chapterMapElement)
            } else {
                console.log("Error loading component:", co.errorString())
            }
        }

        onChapterDeleted : function (uuid) {
            // Remove chapter from map
            let chapterMapElement = win.chapterMapElements[uuid]
            mapView.map.removeMapItemGroup(chapterMapElement)
            win.chapterMapElements[uuid].destroy()
            delete win.chapterMapElements[uuid]
            hideChapterEditor(uuid)
        }

        onProduceStarted: function () {
            fileProgressDialog.open()
        }

        onProduceFinished: function () {
            fileProgressDialog.close()
        }

    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&New Project...")
                onTriggered: filesDialog.open()
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
    }

    InputDataDialog {
        id: filesDialog
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
            fileProgressDialog.open()
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

    FolderDialog {
        id: produceFolderDialog
        visible: false
        title: "Select folder for produced video"
        currentFolder: "file:///tmp"
        onAccepted: {
            raceTreeModel.produce(selectedFolder)
        }
    }


    Dialog {
        id: fileProgressDialog
        anchors.centerIn: Overlay.overlay
        title: "Processing data"
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

    GridLayout {
        id: mainGrid
        columns: 3
        anchors.fill: parent

        // Chapters view
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
                model: raceTreeModel
                delegate: TreeViewDelegate {}
                selectionModel: ItemSelectionModel {
                    onCurrentChanged: function (current, previous) {
                        raceTreeModel.currentChanged(current, previous)
                        win.isRaceSelected = raceTreeModel.isRaceSelected()
                        win.isChapterSelected = raceTreeModel.isChapterSelected()
                        win.selectedName = raceTreeModel.getSelectedName()
                    }
                    onSelectionChanged: function (current, previous) {
                        raceTreeModel.selectionChanged(current, previous)
                    }
                }
            }

            Button {
                id: deleteRaceOrChapterButton
                text: "Delete " + selectedName
                enabled: isRaceSelected || isChapterSelected
                onClicked: {
                    raceTreeModel.deleteSelected()
                }
            }

            Button {
                id: addChapterButton
                text: "Add chapter"
                onClicked: {
                    raceTreeModel.addChapter()
                }
            }

            Button {
                id: addRaceButton
                text: "Split race"
                onClicked: {
                    raceTreeModel.splitRace()
                }
            }
        }

        // Map and chapter editor
        GridLayout {
            id: mapGrid
            columns: 5

            MapView {
                id: mapView
                Layout.columnSpan: 5
                Layout.fillHeight: true
                Layout.fillWidth: true

                Layout.preferredHeight: 400
                Layout.preferredWidth: 640

                Layout.minimumHeight: 200
                Layout.minimumWidth: 320

                map.plugin: Plugin {
                    name: "osm"
                }
                map.zoomLevel: 4
            }

            Button {
                text: "<<"
                onClicked: mediaplayer.moveByMs(-60 * 1000)
            }

            Button {
                text: "<"
                onClicked: mediaplayer.moveByMs(-1000)
            }

            Label {
                id: currentTimeLabel
                text: ""
            }

            Button {
                text: ">"
                onClicked: mediaplayer.moveByMs(1000)
            }

            Button {
                text: ">>"
                onClicked: mediaplayer.moveByMs(60 * 1000)
            }

            ChapterEditor {
                id: chapterEditor
                model: raceTreeModel
                Layout.columnSpan: 5

                onChanged: function () {
                    console.log("Chapter changed: " + chapterName + " " + chapterType + " " + startIdx + " " + endIdx)
                    raceTreeModel.updateChapter(chapterUuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
                    win.chapterMapElements[chapterUuid].path = win.fullMapPath.slice(startIdx, endIdx)
                }

                onStartIdxChanged: {
                    console.log("onStartIdxChanged: " + Math.round(startIdx))
                    let idx = Math.round(startIdx)
                    selectedChapterStartMarker.coordinate = win.fullMapPath[idx]
                    raceTreeModel.seekToRacePathIdx(idx)
                }

                onEndIdxChanged: {
                    console.log("onEndIdxChanged: " + Math.round(endIdx))
                    let idx = Math.round(endIdx)
                    selectedChapterEndMarker.coordinate = win.fullMapPath[idx]
                    raceTreeModel.seekToRacePathIdx(idx)
                }

            }

            RaceEditor {
                id: raceEditor
                Layout.columnSpan: 5

                onChanged: {
                    console.log("Race changed: " + raceName )
                    raceTreeModel.updateRace(raceName)
                }
            }

    }

        // Video and instruments view
        GridLayout {
            columns: 5

            // Video
            Item {
                Layout.columnSpan: 5
                Layout.fillHeight: true
                Layout.fillWidth: true

                Layout.preferredHeight: 400
                Layout.preferredWidth: 848

                Layout.minimumHeight: 200
                Layout.minimumWidth: 424

                GoProMediaPlayer {
                    id: mediaplayer
                    model: raceTreeModel

                    // source: "file:///Volumes/G-DRIVE%20mobile%20USB-C/NPSOLOTWIN/Race/GOPRO/100GOPRO/GX010058.MP4"
                    audioOutput: AudioOutput {}
                    videoOutput: videoOutput

                    // Called by the player during the playback
                    onRacePathIdxChanged: function (idx) {
                        // Update progress slider
                        progressSlider.value = (idx - win.raceStartidx) / win.raceLength
                        raceTreeModel.seekToRacePathIdx(idx)
                    }

                    // Called by the player during the playback
                    onPositionChanged: function () {
                        // Update time label
                        let posSt = new Date(position).toISOString().substring(14, 19)
                        let durSt = new Date(duration).toISOString().substring(14, 19)

                        clipPositionLabel.text = posSt + " / " + durSt
                    }

                    onErrorOccurred: function (error, errorString) {
                        console.log('error=', error, 'errorString=', errorString)
                    }

                    onClipNameChanged: function (clipName) {
                        clipNameLabel.text = clipName
                    }

                }
                VideoOutput {
                    id: videoOutput
                    anchors.fill: parent
                }

                Item {
                    height: 50
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 20

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: mediaplayer.playbackState ===  MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
                        onClicked: {
                            switch(mediaplayer.playbackState) {
                                case MediaPlayer.PlayingState: mediaplayer.pauseClip(); break;
                                case MediaPlayer.PausedState: mediaplayer.playClip(); break;
                                case MediaPlayer.StoppedState: mediaplayer.playClip(); break;
                            }
                        }
                    }

                    Slider {
                        id: progressSlider
                        width: parent.width
                        anchors.bottom: parent.bottom
                        enabled: true
                        background: Rectangle {
                            implicitHeight: 8
                            color: "white"
                            radius: 3
                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#1D8BF8"
                                radius: 3
                            }
                        }
                        handle: Item {}
                        onMoved: function () {
                            let idx = Math.round(win.raceStartidx + progressSlider.position * win.raceLength)
                            mediaplayer.seekTo(idx)
                            raceTreeModel.seekToRacePathIdx(idx)
                        }
                    }
                }
            }

            Label {
                id: clipNameLabel
                text: ""
            }
            Label {
                id: clipPositionLabel
                text: ""
            }
        }

    }

}
