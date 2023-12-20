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

    property string chapterName
    property int chapterType
    property int chapterStartIdx
    property int chapterEndIdx
    property int chapterGunIdx

    property int raceLength: 0
    property int raceStartidx: 0

    property real videoSliderX: 8
    property real videoSliderWidth: top_panel.width - 16

    property real scrollOffset: 0
    property real scaleFactor: 1.0

    function hideChapterEditor(uuid) {
        chapterEditor.visible = false
    }

    function idxToX(idx) {
        let beforeScrollX = (idx - raceStartidx) / raceLength * videoSlider.width;
        return (beforeScrollX - scrollOffset) * scaleFactor
    }

    function xToIdx(x) {
        let beforeScrollX = x / scaleFactor + scrollOffset
        return beforeScrollX / videoSlider.width * raceLength + raceStartidx
    }

    function updateChapterSliderGeometry() {
        chapterSlider.x = idxToX(chapterStartIdx) + videoSliderX
        chapterSlider.width = idxToX(chapterEndIdx) - idxToX(chapterStartIdx)

        gunIndicator.x = idxToX(chapterGunIdx) - chapterSlider.x - gunIndicator.width/2
        gunIndicator.visible = chapterType === ChapterTypes.START || chapterType === ChapterTypes.MARK_ROUNDING
    }

    function onRacePathIdxChanged(idx) {
        currentTimeLabel.text = raceTreeModel.getTimeString(idx)
        // Update progress slider
        progressIndicator.currentIdx = idx
    }

    function onRaceSelected(startIdx, endIdx){
        raceLength = endIdx - startIdx
        raceStartidx = startIdx
        progressIndicator.currentIdx = startIdx
    }

    function onChapterSelected (chapter_uuid, name, type, start_idx, end_idx, gun_idx) {
        console.log("RaceTimeline: onChapterSelected", chapter_uuid, name, type, start_idx, end_idx, gun_idx)
        chapterSlider.visible = true

        chapterName = name
        chapterType = type
        chapterStartIdx = start_idx
        chapterEndIdx = end_idx
        chapterGunIdx = gun_idx

        chapterEditor.visible = true

        // Position the chapter indicator and gun indicator
        // We position them here, since we can not bind x to idx in the object property,
        // since the binding will be removed when we change x when dragging mose
        updateChapterSliderGeometry()
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
        id: buttonZoomIn
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
            if ( scaleFactor < 16 ){
                scaleFactor = scaleFactor * 2
                updateChapterSliderGeometry()
            }
        }
    }

    Button{
        id: buttonZoomOut
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
            updateChapterSliderGeometry()
        }
    }

    // Chapter information
    ChapterEditor {
        id: chapterEditor
        anchors.top: buttonZoomIn.top
        anchors.bottom: buttonZoomIn.bottom
        anchors.right: buttonZoomIn.left
        anchors.rightMargin: 32

        startIdx: chapterStartIdx
        endIdx: chapterEndIdx
        gunIdx: chapterGunIdx
        chapterName: top_panel.chapterName
        chapterType: top_panel.chapterType

        model: top_panel.model
    }

    // Video slider
    Rectangle {
        id: videoSlider
        anchors.top: currentTimeLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        implicitHeight: 8

        x: videoSliderX * scaleFactor
        width: videoSliderWidth * scaleFactor

        color: "red"
        radius: 3
        Rectangle {
            id: progressIndicator
            property real currentIdx: 0
            x: idxToX(currentIdx)  - width / 2
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
                        let idx = xToIdx(progressIndicator.x + width / 2)
                        player.seekTo(idx)
                        raceTreeModel.seekToRacePathIdx(idx)
                    }
                }
            }
        }
    }

    // Chapter slider
    Rectangle {
        id: chapterSlider
        anchors.top: videoSlider.bottom
        anchors.margins: 8
        radius: 3
        implicitHeight: 24

        visible: false

        color: "#15814b"

        // onXChanged: {
        //     console.log("chapterSlider.x: " + chapterSlider.x)
        // }
        // onWidthChanged: {
        //     console.log("chapterSlider.width: " + chapterSlider.width)
        // }

        // Gun indicator
        Rectangle {
            id: gunIndicator
            width: 10
            radius: 3
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
                        chapterGunIdx = xToIdx(chapterSlider.x + gunIndicator.x)
                    }
                }
            }
        }

        // Start indicator
        Rectangle {
            id: startIndicator
            width: 10
            radius: 3
            x: 0
            anchors.top: chapterSlider.top
            anchors.bottom: chapterSlider.bottom
            color: "#00000000"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                drag.target: startIndicator
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: chapterSlider.width - startIndicator.width
                onMouseXChanged: {
                    if ( drag.active) {
                        // Update view
                        let newWidth = chapterSlider.width - mouseX
                        if (newWidth < startIndicator.width)
                            return

                        chapterSlider.x = chapterSlider.x + mouseX
                        chapterSlider.width = newWidth
                        // Update dependencies
                        chapterStartIdx = xToIdx(chapterSlider.x)
                        // Keep gun inside of the chapter
                        if (chapterGunIdx < chapterStartIdx) {
                            chapterGunIdx = chapterStartIdx
                        }
                    }
                }
            }
        }

        // End indicator
        Rectangle {
            id: endIndicator
            width: 10
            radius: 3
            x: chapterSlider.width - width
            anchors.top: chapterSlider.top
            anchors.bottom: chapterSlider.bottom
            color: "#00000000"

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
                        chapterEndIdx = xToIdx(chapterSlider.x + chapterSlider.width)
                        if (chapterGunIdx > chapterEndIdx) {
                            chapterGunIdx = chapterEndIdx
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
            updateChapterSliderGeometry()
        }
    }

}