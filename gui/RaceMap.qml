import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtLocation
import QtPositioning
import sails
import QtQuick.Controls.Fusion

Rectangle {
    anchors.fill: parent
    required property var model
    property var fullMapPath  // Path for entire project
    property var chapterMapElements: {'aa': 'bb'}  // Initialize it with some nonsense, otherwise we get an assignment error

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

        MapQuickItem {
            id: selectedChapterGunMarker
            anchorPoint.x: endImage.width / 2
            anchorPoint.y: endImage.height / 2
            coordinate: QtPositioning.coordinate(0, 0)

            sourceItem: Rectangle {
                id: gunImage
                width: 9
                height: 18
                color: "green"
                radius: 9
            }
        }
    }

    MapView {
        id: mapView
        visible: true
        anchors.fill: parent
        map.plugin: Plugin {
            name: "osm"
        }
        map.zoomLevel: 4
    }

    function onFullPathReady(raceGeoPath) {
        fullMapPath = raceGeoPath.path
        racePathLine.path = raceGeoPath.path

        mapView.map.addMapItem(racePathLine)
        mapView.map.addMapItem(boatMarker)
        mapView.map.addMapItemGroup(selectedChapterMarkers)
        mapView.map.center = raceGeoPath.path[0]
        boatMarker.coordinate = fullMapPath[0]
    }

    function onRaceSelected(startIdx, endIdx){
        racePathLine.path = fullMapPath.slice(startIdx, endIdx)

        // Clear all chapter map elements
        for(let uuid in chapterMapElements) {
            if ( uuid === "aa")  continue  // FIXME need to rid of stupid initializer in the property declaration
            console.log("Removing Chapter map element uuid: " + uuid)
            const chapterMapElement = chapterMapElements[uuid]
            console.log("Removing chapter map element: " + chapterMapElement)
            mapView.map.removeMapItemGroup(chapterMapElement)
            chapterMapElements[uuid].destroy()
            delete chapterMapElements[uuid]
        }

    }

    function onChapterAdded(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx){
        let co = Qt.createComponent('ChapterMapElement.qml')
        if (co.status === Component.Ready) {
            let chapterMapElement = co.createObject(appWindow)
            chapterMapElement.path = fullMapPath.slice(startIdx, endIdx)

            if ( chapterType === ChapterTypes.START || chapterType === ChapterTypes.MARK_ROUNDING ) {
                chapterMapElement.evt_visible = true
                chapterMapElement.evt_coord = fullMapPath[gunIdx]
            } else {
                chapterMapElement.evt_visible = false
            }

            chapterMapElements[uuid] = chapterMapElement
            mapView.map.addMapItemGroup(chapterMapElement)
        } else {
            console.log("Error loading component:", co.errorString())
        }
    }
    function onChapterDeleted(uuid) {
        // Remove chapter from map
        let chapterMapElement = chapterMapElements[uuid]
        mapView.map.removeMapItemGroup(chapterMapElement)
        chapterMapElements[uuid].destroy()
        delete chapterMapElements[uuid]
    }


    function onChapterSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
        selectedChapterMarkers.visible = true
        selectedChapterStartMarker.coordinate = fullMapPath[startIdx]
        selectedChapterEndMarker.coordinate = fullMapPath[endIdx]
        selectedChapterGunMarker.coordinate = fullMapPath[gunIdx]

        selectedChapterGunMarker.visible =
            chapterType === ChapterTypes.START
            || chapterType === ChapterTypes.MARK_ROUNDING;

    }

    function onRacePathIdxChanged(idx) {
        boatMarker.coordinate = fullMapPath[idx]
    }

    function updateChapter(chapterUuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
        chapterMapElements[chapterUuid].path = fullMapPath.slice(startIdx, endIdx)
    }

    function onSelectedChapterStartIdxChanged(idx) {
        selectedChapterStartMarker.coordinate = fullMapPath[idx]
    }

    function onSelectedChapterEndIdxChanged(idx) {
        selectedChapterEndMarker.coordinate = fullMapPath[idx]
    }

    function onSelectedChapterGunIdxChanged(idx) {
        selectedChapterGunMarker.coordinate = fullMapPath[idx]
    }

}