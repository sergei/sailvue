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
    id: top
    anchors.fill: parent
    required property var model

    property string selectedChapterUuid: ""
    property int selectedChapterStartIdx: 0
    property int selectedChapterEndIdx: 0
    property int selectedChapterGunIdx: 0

    property var fullMapPath  // Path for entire project
    property var chapterMapElements: {'aa': 'bb'}  // Initialize it with some nonsense, otherwise we get an assignment error

    MapPolyline {
        id: racePathLine
        line.width: 3
        line.color: "#5b5a5a"
    }

    MapQuickItem {
        id: boatMarker
        anchorPoint.x: boatImage.width / 2
        anchorPoint.y: boatImage.height / 2

        sourceItem: Image {
            id: boatImage
            width: 32
            height: 32
            source: "qrc:/images/mapmarker.png"
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
        console.log('fullMapPath.length=', fullMapPath.length)
        racePathLine.path = raceGeoPath.path

        mapView.map.addMapItem(racePathLine)
        mapView.map.addMapItem(boatMarker)
        mapView.map.center = raceGeoPath.path[0]
        boatMarker.coordinate = fullMapPath[0]

        mapView.map.fitViewportToVisibleMapItems()
    }

    function onRaceSelected(startIdx, endIdx){
        racePathLine.path = fullMapPath.slice(startIdx, endIdx)
    }

    function onRaceUnSelected(startIdx, endIdx){
        // Clear all chapter map elements
        for(let uuid in chapterMapElements) {
            if ( uuid === "aa")  continue  // FIXME need to rid of stupid initializer in the property declaration
            console.log('Removing chapterMapElements[uuid]', uuid)
            const chapterMapElement = chapterMapElements[uuid]
            mapView.map.removeMapItemGroup(chapterMapElement)
            chapterMapElements[uuid].destroy()
            delete chapterMapElements[uuid]
        }
    }

    function onChapterAdded(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx){
        console.log('onChapterAdded', uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)
        let co = Qt.createComponent('ChapterMapElement.qml')
        console.log(co)
        if (co.status === Component.Ready) {

            let chapterMapElement = co.createObject(appWindow)
            chapterMapElement.path = fullMapPath.slice(startIdx, endIdx)
            chapterMapElement.chapterType = chapterType
            chapterMapElement.evt_coord = fullMapPath[gunIdx]

            chapterMapElements[uuid] = chapterMapElement
            mapView.map.addMapItemGroup(chapterMapElement)
        } else {
            console.log("Error loading component:", co.errorString())
        }

        console.log('onChapterAdded chapterMapElements size', Object.keys(chapterMapElements).length)
    }

    function onChapterDeleted(uuid) {
        console.log("RaceMap:  onChapterDeleted", uuid)
        // Remove chapter from map
        let chapterMapElement = chapterMapElements[uuid]
        mapView.map.removeMapItemGroup(chapterMapElement)
        chapterMapElements[uuid].destroy()
        delete chapterMapElements[uuid]
    }
    
    function onChapterSelected(uuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
        console.log("RaceMap:  onChapterSelected", uuid, chapterName, chapterType, startIdx, endIdx, gunIdx)

        let prevUuid = selectedChapterUuid
        selectedChapterUuid = uuid

        console.log("prevUuid ", prevUuid)
        console.log("selectedChapterUuid ", selectedChapterUuid)

        if ( prevUuid !== "" && chapterMapElements[prevUuid] !== undefined){
            chapterMapElements[prevUuid].selected = false
        }

        selectedChapterStartIdx = startIdx
        selectedChapterEndIdx = endIdx
        selectedChapterGunIdx = gunIdx

        chapterMapElements[selectedChapterUuid].selected = true
    }

    function onRacePathIdxChanged(idx) {
        boatMarker.coordinate = fullMapPath[idx]
    }

    function updateChapter(chapterUuid, chapterName, chapterType, startIdx, endIdx, gunIdx) {
        chapterMapElements[chapterUuid].path = fullMapPath.slice(startIdx, endIdx)
    }

    function onSelectedChapterStartIdxChanged(idx) {
        selectedChapterStartIdx = idx
        console.log("onSelectedChapterStartIdxChanged selectedChapterUuid ", selectedChapterUuid, selectedChapterStartIdx, selectedChapterEndIdx)
        chapterMapElements[selectedChapterUuid].path = fullMapPath.slice(selectedChapterStartIdx, selectedChapterEndIdx)
    }

    function onSelectedChapterEndIdxChanged(idx) {
        selectedChapterEndIdx = idx
        console.log("onSelectedChapterEndIdxChanged selectedChapterUuid ", selectedChapterUuid, selectedChapterStartIdx, selectedChapterEndIdx)
        chapterMapElements[selectedChapterUuid].path = fullMapPath.slice(selectedChapterStartIdx, selectedChapterEndIdx)
    }

    function onSelectedChapterGunIdxChanged(idx) {
        chapterMapElements[selectedChapterUuid].evt_coord = fullMapPath[idx]
    }

}