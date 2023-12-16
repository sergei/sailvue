import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import sails

GridLayout {
    required property var model

    property string chapterName
    property int chapterType
    property int startIdx
    property int endIdx
    property int gunIdx

    visible: false

    Label {
        id: chapter_name
        text: chapterName
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
        visible: chapterType === ChapterTypes.START || chapterType === ChapterTypes.MARK_ROUNDING
        text: chapterType === ChapterTypes.START ? "Gun at:" : "Mark at:"
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
