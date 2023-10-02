import QtQuick
import QtPositioning
import QtLocation


MapItemGroup {

    property alias path: chapterPathLine.path
    property alias evt_coord: eventMarker.coordinate
    property alias evt_visible: eventMarker.visible

    onPathChanged: {
        let path = chapterPathLine.path
        let start = path[0]
        let end = path[path.length - 1]

        startMarker.coordinate = start
        endMarker.coordinate = end
    }

    MapQuickItem {
        id: startMarker
        anchorPoint.x: startImage.width / 2
        anchorPoint.y: startImage.height / 2

        sourceItem: Rectangle {
            id: startImage
            width: 12
            height: 12
            color: "red"
            radius: 6
        }
    }

    MapQuickItem {
        id: eventMarker
        visible: false
        anchorPoint.x: endImage.width / 2
        anchorPoint.y: endImage.height / 2

        sourceItem: Rectangle {
            id: eventImage
            width: 10
            height: 10
            color: "green"
            radius: 5
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
            color: "yellow"
            radius: 6
        }
    }

    MapPolyline {
        id: chapterPathLine
        line.width: 3
        line.color: "blue"
        path: []
    }

}
