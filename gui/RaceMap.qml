import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtLocation

import sails

Rectangle {
    anchors.fill: parent
    required property var model

    MapView {
        id: mapView
        visible: true
        anchors.fill: parent
        map.plugin: Plugin {
            name: "osm"
        }
        map.zoomLevel: 4
    }

}