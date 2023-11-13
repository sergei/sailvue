import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Fusion

Rectangle {
    anchors.fill: parent
    color: "#5b5a5a"
    id: top_panel

    required property var model
    required property var player

    signal chapterChanged()
    property alias chapterStartIdx: chapterEditor.startIdx
    property alias chapterEndIdx: chapterEditor.endIdx
    property alias chapterGunIdx: chapterEditor.gunIdx

    property alias raceName: raceEditor.raceName

    property int raceLength: 0
    property int raceStartidx: 0

    property int selectedChapterStartIdx: 0
    property int selectedChapterEndIdx: 0
    property int selectedChapterGunIdx: 0

    property real videoSliderX: 8
    property real videoSliderWidth: top_panel.width - 16

    property int chapterLength : 0

    property real chapterSliderX: 100
    property real chapterSliderWidth: 200

    property real scrollOffset: 0
    property real scaleFactor: 1.0

    function hideChapterEditor(uuid) {
        console.log("Hiding chapter editor")
        raceEditor.visible = false
        chapterEditor.visible = false
    }

    function onRacePathIdxChanged(idx) {
        currentTimeLabel.text = raceTreeModel.getTimeString(idx)
        // Update progress slider
        progressIndicator.position = (idx - raceStartidx) / raceLength
    }

    function onRaceSelected(startIdx, endIdx){
        raceLength = endIdx - startIdx
        raceStartidx = startIdx
        progressIndicator.position = 0
    }

    function onChapterSelected (chapter_uuid, name, chapterType, start_idx, end_idx, gun_idx) {
        console.log("onChapterSelected: [" + name + "], type=" + chapterType + ", start_idx=" + start_idx + ", end_idx=" + end_idx, ", gun_idx=" + gun_idx)

        selectedChapterStartIdx = start_idx
        selectedChapterEndIdx = end_idx
        selectedChapterGunIdx = gun_idx

        chapterLength = end_idx - start_idx
        chapterSliderWidth = chapterLength / raceLength * videoSlider.width
        chapterSliderX = (start_idx - raceStartidx) / raceLength * videoSlider.width

        console.log("chapterLength=" + chapterLength + ", chapterSlider.width=" + chapterSlider.width + ", chapterSlider.x=" + chapterSlider.x)

        chapterEditor.visible = true
        chapterEditor.setSelected(chapter_uuid, name, chapterType, start_idx, end_idx, gun_idx)
    }

    function onChapterUnSelected(uuid) {
        hideChapterEditor(uuid)
    }

     function onChapterDeleted (uuid) {
        hideChapterEditor(uuid)
    }

    Label {
        id: currentTimeLabel
        anchors.margins: 8
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "--:--:--"
    }

    Button{
        anchors.right: currentTimeLabel.left
        anchors.verticalCenter: currentTimeLabel.verticalCenter
        anchors.margins: 8
        width: 32
        height: 32
        Image {
            anchors.fill: parent
            source: "qrc:/images/icons8-zoom-in-50.png"
        }
        onClicked: {
            if ( scaleFactor < 16 )
                scaleFactor = scaleFactor * 2
        }
    }

    Button{
        anchors.left: currentTimeLabel.right
        anchors.verticalCenter: currentTimeLabel.verticalCenter
        anchors.margins: 8
        width: 32
        height: 32
        Image {
            anchors.fill: parent
            source: "qrc:/images/icons8-zoom-out-50.png"
        }
        onClicked: {
            scaleFactor = scaleFactor / 2
            if ( scaleFactor < 1 )
                scaleFactor = 1
        }
    }

    Rectangle {
        id: videoSlider
        anchors.top: currentTimeLabel.bottom

        x: videoSliderX * scaleFactor
        width: videoSliderWidth * scaleFactor

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        implicitHeight: 8
        color: "red"
        radius: 3
        Rectangle {
            id: progressIndicator
            property real position: 0
            x: (videoSliderWidth  * position - scrollOffset) * scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            width: 20
            height: 20
            radius: 20
            color: "#1D8BF8"

            MouseArea {
                anchors.fill: parent
                drag.target: progressIndicator
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: videoSlider.width - progressIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        progressIndicator.position = progressIndicator.x / videoSlider.width
                        let idx = Math.round(raceStartidx + progressIndicator.position * raceLength)
                        player.seekTo(idx)
                        raceTreeModel.seekToRacePathIdx(idx)
                    }
                }
            }
        }
    }

    Rectangle {
        id: chapterSlider
        anchors.top: videoSlider.bottom
        anchors.margins: 8
        radius: 3
        implicitHeight: 24
        x: (chapterSliderX - scrollOffset) * scaleFactor
        width: chapterSliderWidth * scaleFactor
        color: "#15814b"

        onXChanged: {
            console.log("chapterSlider.x=" + x)
        }

        Rectangle {
            id: gunIndicator
            property real position: 0.5
            width: 10
            radius: 3
            x: parent.width * position - width / 2
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: "#072c1a"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: gunIndicator
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: chapterSlider.width - gunIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        // gunIndicator.gunPosition = gunIndicator.x / gunIndicator.width
                        // let idx = Math.round(selectedChapterStartIdx + gunIndicator.position * (selectedChapterEndIdx - selectedChapterStartIdx))
                    }
                }
            }
        }

        Rectangle {
            id: startIndicator
            property real position: 0.5
            width: 10
            radius: 3
            x: 0
            anchors.top: chapterSlider.top
            anchors.bottom: chapterSlider.bottom
            color: "#2bf191"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: startIndicator
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: chapterSlider.width / 2 - startIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        chapterSlider.x = chapterSlider.x + mouseX
                        let newWidth = chapterSlider.width - mouseX
                        if (newWidth < 30)
                            return
                        chapterSlider.width = newWidth
                    }
                }
            }
        }

        Rectangle {
            id: endIndicator
            property real position: 0.5
            width: 10
            radius: 3
            x: chapterSlider.width - width
            anchors.top: chapterSlider.top
            anchors.bottom: chapterSlider.bottom
            color: "#2bf191"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: endIndicator
                drag.axis: Drag.XAxis
                drag.minimumX: chapterSlider.width/2 + endIndicator.width
                drag.maximumX: chapterSlider.width - endIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        let newWidth = chapterSlider.width + mouseX
                        if (newWidth < 30)
                            return
                        chapterSlider.width = newWidth
                    }
                }
            }
        }

    }

    ScrollBar {
        id: hbar
        anchors.left: videoSlider.left
        anchors.right: videoSlider.right
        anchors.top: chapterSlider.bottom
        hoverEnabled: true
        active: true
        // active: hovered || pressed
        orientation: Qt.Horizontal
        size: 1 / scaleFactor

        onPositionChanged: {
            scrollOffset = top_panel.width  * position
            console.log("hbar.position=" + position + ", scrollOffset=" + scrollOffset)
        }
    }

    Rectangle {
        id: chapterAndRaceEditors
        anchors.top: hbar.bottom
        anchors.margins: 12
        color: "#ce0e0e"

        ChapterEditor {
            id: chapterEditor
            model: top_panel.model

            onChanged: function () {
                chapterChanged()
            }
        }
    }


    RaceEditor {
        id: raceEditor
        onChanged: {
            raceTreeModel.updateRace(raceName)
        }
        onDetectManeuvers: {
            raceTreeModel.detectManeuvers()
        }
        onMakeAnalytics: {
            raceTreeModel.makeAnalytics()
        }
    }

}