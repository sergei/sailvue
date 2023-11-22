import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Fusion
import sails


Rectangle {
    anchors.fill: parent
    color: "#5b5a5a"
    id: top_panel

    required property var model
    required property var player

    property alias chapterUuid: chapterEditor.chapterUuid
    property alias chapterStartIdx: chapterEditor.startIdx
    property alias chapterEndIdx: chapterEditor.endIdx
    property alias chapterGunIdx: chapterEditor.gunIdx
    property alias chapterName: chapterEditor.chapterName
    property alias chapterType: chapterEditor.chapterType
    signal chapterChanged()

    property alias raceName: raceEditor.raceName

    property int raceLength: 0
    property int raceStartidx: 0

    property real videoSliderX: 8
    property real videoSliderWidth: top_panel.width - 16

    property real scrollOffset: 0
    property real scaleFactor: 1.0


    function hideChapterEditor(uuid) {
        raceEditor.visible = false
        chapterEditor.visible = false
    }

    function chapterIdxToX(idx, offset, scale) {
        let beforeScrollX = (idx - raceStartidx) / raceLength * videoSlider.width;
        return (beforeScrollX - offset) * scale
    }

    function chapterXToIdx(x, offset, scale) {
        let beforeScrollX = x / scale + offset
        return beforeScrollX / videoSlider.width * raceLength + raceStartidx
    }

    function onRacePathIdxChanged(idx) {
        currentTimeLabel.text = raceTreeModel.getTimeString(idx)
        // Update progress slider
        progressIndicator.position = (idx - raceStartidx) / raceLength
    }

    function onRaceSelected(startIdx, endIdx){
        raceEditor.visible = true
        raceLength = endIdx - startIdx
        raceStartidx = startIdx
        progressIndicator.position = 0
    }

    function onChapterSelected (chapter_uuid, name, chapterType, start_idx, end_idx, gun_idx) {
        console.log("onChapterSelected: [" + name + "], type=" + chapterType + ", start_idx=" + start_idx + ", end_idx=" + end_idx, ", gun_idx=" + gun_idx)

        chapterSlider.visible = true
        chapterEditor.startIdx = start_idx
        chapterEditor.endIdx = end_idx
        chapterEditor.gunIdx = gun_idx

        gunIndicator.visible = chapterType === ChapterTypes.START
            || chapterType === ChapterTypes.MARK_ROUNDING

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

        visible: false

        x: chapterIdxToX(chapterEditor.startIdx, scrollOffset, scaleFactor)
        width: chapterIdxToX(chapterEditor.endIdx, scrollOffset, scaleFactor)
             - chapterIdxToX(chapterEditor.startIdx, scrollOffset, scaleFactor)
        color: "#15814b"

        onXChanged: {
            console.log("chapterSlider.x: " + chapterSlider.x)
        }
        onWidthChanged: {
            console.log("chapterSlider.width: " + chapterSlider.width)
        }

        Rectangle {
            id: gunIndicator
            property real position: 0.5
            width: 10
            radius: 3
            x: chapterIdxToX(chapterEditor.gunIdx, scrollOffset, scaleFactor) - chapterSlider.x - width/2
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
                        chapterEditor.gunIdx = chapterXToIdx(chapterSlider.x + gunIndicator.x, scrollOffset, scaleFactor)
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
                drag.maximumX: chapterSlider.width - startIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        console.log("mouseX: " + mouseX + ", startIndicator.x: " + startIndicator.x )

                        // Update view
                        let newWidth = chapterSlider.width - mouseX
                        if (newWidth < startIndicator.width)
                            return

                        chapterSlider.x = chapterSlider.x + mouseX
                        chapterSlider.width = newWidth
                        // Update dependencies
                        chapterEditor.startIdx = chapterXToIdx(chapterSlider.x, scrollOffset, scaleFactor)
                        // Keep gun inside of the chapter
                        if (chapterEditor.gunIdx < chapterEditor.startIdx) {
                            chapterEditor.gunIdx = chapterEditor.startIdx
                        }
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
                drag.minimumX: 0
                drag.maximumX: chapterSlider.width - endIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        // Update view
                        let newWidth = chapterSlider.width + mouseX
                        if (newWidth < endIndicator.width)
                            return
                        chapterSlider.width = newWidth

                        // Update dependencies
                        chapterEditor.endIdx = chapterXToIdx(chapterSlider.x + chapterSlider.width, scrollOffset, scaleFactor)
                        if (chapterEditor.gunIdx > chapterEditor.endIdx) {
                            chapterEditor.gunIdx = chapterEditor.endIdx
                        }
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
        }
    }

    Rectangle {
        id: chapterAndRaceEditors
        anchors.top: hbar.bottom
        anchors.margins: 12
        color: "#ce0e0e"

        RaceEditor {
            id: raceEditor
            anchors.left: parent.left
            anchors.leftMargin: 12

            onChanged: {
                model.updateRace(raceName)
            }
            onDetectManeuvers: {
                model.detectManeuvers()
            }
            onMakeAnalytics: {
                model.makeAnalytics()
            }
        }

        ChapterEditor {
            id: chapterEditor
            anchors.top: raceEditor.bottom
            anchors.left: parent.left
            anchors.leftMargin: 12

            model: top_panel.model

            onChanged: function () {
                chapterChanged()
            }
        }
    }
}