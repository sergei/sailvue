import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import sails

GridLayout {

    columns: 5
    required property var model

    property string chapterUuid
    property string chapterName
    property int chapterType
    property int startIdx
    property int endIdx
    property int gunIdx

    property int fullRange
    property int rangeStart

    visible: false
    signal changed()
    signal gunSet()

    function changeGunCtrlVisibility(chapterTypeIdx) {
        if (chapterTypeIdx === ChapterTypes.START || chapterTypeIdx === ChapterTypes.MARK_ROUNDING) {
            // Show gun controls
            gun_label.visible = true
            gun_idx.visible = true
            gun_set.visible = true

            if (chapterTypeIdx === ChapterTypes.START){
                gun_label.text = "Gun at"
                gun_set.text = "Set gun"
            }else if (chapterTypeIdx === ChapterTypes.MARK_ROUNDING){
                gun_label.text = "Mark at"
                gun_set.text = "Set mark"
            }

        } else {
            // Hide gun controls
            gun_label.visible = false
            gun_idx.visible = false
            gun_set.visible = false
        }
    }

    function setSelected (chapter_uuid, name, type, start_idx, end_idx, gun_idx) {
        console.log("Chapter Selected: [" + name + "], type=" + chapterType + ", start_idx=" + start_idx + ", end_idx=" + end_idx, ", gun_idx=" + gun_idx)
        chapterUuid = chapter_uuid
        chapterName = name
        chapter_name.text = chapterName
        chapterType = type
        start_time.text = model.getTimeString(start_idx)
        end_time.text = model.getTimeString(end_idx)
        chapter_type.currentIndex = chapterType

        slider.from = 0
        slider.to = 1
        slider.first.value = 0.25
        slider.second.value = 0.75

        startIdx = start_idx
        endIdx = end_idx
        gunIdx = gun_idx
        fullRange = (end_idx - start_idx) * 2
        rangeStart = start_idx - fullRange * slider.first.value

        changeGunCtrlVisibility(chapterType)
    }

    RangeSlider {
        id: slider
        Layout.columnSpan: 5
        Layout.fillWidth: true

        first.onMoved: {
            startIdx = rangeStart + first.value * fullRange
            if ( startIdx < 0 ) {
                startIdx = 0
            }
            start_time.text = model.getTimeString(startIdx)
        }

        second.onMoved: {
            endIdx = rangeStart + second.value * fullRange
            // FIXME check out of range value
            end_time.text = model.getTimeString(endIdx)
        }

    }

    Label {
        id: start_time
        text: "From"
    }

    Label {
        id: end_time
        text: "To"
    }

    TextEdit {
        id: chapter_name
        text: "Tack"
        color: "white"
        onTextChanged: {
            parent.chapterName = text
        }
    }

    ComboBox {
        id: chapter_type
        model: ["Tack/Gybe", "Performance", "Start", "Mark rounding"]
        onActivated: function (index) {
            console.log("Chapter type changed to " + index)
            parent.chapterType = index
            changeGunCtrlVisibility(index);
        }
    }

    Button {
        text: "Save"
        onClicked: {
            changed()
        }
    }

    Label {
        id: gun_label
        text: "Gun at"
    }

    Label {
        id: gun_idx
        text: model.getTimeString(gunIdx)
    }

    Button {
        id: gun_set
        text: "Set gun"
        onClicked: {
            gunSet()
        }
    }

}
