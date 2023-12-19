import QtQuick
import QtPositioning
import QtLocation
import sails

MapItemGroup {

    property alias path: chapterPathLine.path
    property alias evt_coord: eventMarker.coordinate
    property var selected: false
    property var chapterType:  ChapterTypes.TACK_GYBE

    function getPathColor(){

        if ( selected ){
            return "yellow"
        }

        switch (chapterType) {
            case ChapterTypes.TACK_GYBE:
                return  "purple"
            case ChapterTypes.SPEED_PERFORMANCE:
                return  "#cc6816"
            case ChapterTypes.START:
                return "green"
            case ChapterTypes.MARK_ROUNDING:
                return  "blue"
        }
    }

    function eventMarkIsVisible(){
        return chapterType === ChapterTypes.START || chapterType === ChapterTypes.MARK_ROUNDING
    }

    onPathChanged: {
        let path = chapterPathLine.path
        let start = path[0]
        let end = path[path.length - 1]

        startMarker.coordinate = start
        endMarker.coordinate = end
    }

    MapPolyline {
        id: chapterPathLine
        line.width: selected ? 12 : 6
        line.color: getPathColor()
        path: []
    }

    MapQuickItem {
        id: startMarker
        anchorPoint.x: startImage.width / 2
        anchorPoint.y: startImage.height / 2

        sourceItem: Rectangle {
            id: startImage
            width: 12
            height: 12
            color: getPathColor()
            radius: 6
        }
    }

    MapQuickItem {
        id: eventMarker
        visible: eventMarkIsVisible()
        anchorPoint.x: endImage.width / 2
        anchorPoint.y: endImage.height / 2

        sourceItem: Rectangle {
            id: eventImage
            width: selected ? 20 : 10
            height: selected ? 20 : 10
            color: "green"
            radius: width / 2
        }
    }

    MapQuickItem {
        id: endMarker
        anchorPoint.x: endImage.width / 2
        anchorPoint.y: endImage.height / 2

        sourceItem: Rectangle {
            id: endImage
            width: 12
            height: 12
            color: getPathColor()
            radius: 6
        }
    }

}
