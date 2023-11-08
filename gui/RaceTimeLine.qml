import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Fusion

Rectangle {
    anchors.fill: parent
    color: "#5b5a5a"

    required property var model
    required property var player

    property int raceLength: 0
    property int raceStartidx: 0

    Label {
        id: currentTimeLabel
        text: ""
    }

    function onRacePathIdxChanged(idx) {
        currentTimeLabel.text = raceTreeModel.getTimeString(idx)
        // Update progress slider
        progressSlider.value = (idx - raceStartidx) / raceLength
    }

    function onRaceSelected(startIdx, endIdx){
        raceLength = endIdx - startIdx
        raceStartidx = startIdx
        progressSlider.value = 0
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
            let idx = Math.round(raceStartidx + progressSlider.position * raceLength)
            player.seekTo(idx)
            raceTreeModel.seekToRacePathIdx(idx)
        }
    }


}