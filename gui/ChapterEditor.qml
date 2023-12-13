import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import sails

GridLayout {
    required property var model

    property string chapterUuid
    property string chapterName
    property int chapterType
    property int startIdx
    property int endIdx
    property int gunIdx

    visible: false
    signal changed()

    function changeGunCtrlVisibility(chapterTypeIdx) {
        if (chapterTypeIdx === ChapterTypes.START || chapterTypeIdx === ChapterTypes.MARK_ROUNDING) {
            // Show gun controls
            gun_label.visible = true
            gun_idx.visible = true

            if (chapterTypeIdx === ChapterTypes.START){
                gun_label.text = "Gun at:"
            }else if (chapterTypeIdx === ChapterTypes.MARK_ROUNDING){
                gun_label.text = "Mark at:"
            }

        } else {
            // Hide gun controls
            gun_label.visible = false
            gun_idx.visible = false
        }
    }

    function setSelected (chapter_uuid, name, type, start_idx, end_idx, gun_idx) {
        console.log("Chapter Selected: [" + name + "], type=" + chapterType + ", start_idx=" + start_idx + ", end_idx=" + end_idx, ", gun_idx=" + gun_idx)
        chapterUuid = chapter_uuid
        chapterName = name
        chapter_name.text = chapterName
        chapterType = type

        startIdx = start_idx
        endIdx = end_idx
        gunIdx = gun_idx

        changeGunCtrlVisibility(chapterType)
    }

    Label {
        id: chapter_name
        text: "Tack"
    }

    Label {
        text: "From:"
    }

    Label {
        id: start_time
        text: model.getTimeString(startIdx)
    }

    Label {
        id: gun_label
        text: "Gun at:"
    }

    Label {
        id: gun_idx
        text: model.getTimeString(gunIdx)
    }

    Label {
        text: "To:"
    }

    Label {
        id: end_time
        text: model.getTimeString(endIdx)
    }

}
