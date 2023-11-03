import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtMultimedia

import sails

Rectangle {
    anchors.fill: parent
    required property var model

    MouseArea {
        anchors.fill: parent
        onPressed:{
            mediaplayer.play();
            console.log("play");
        }
    }

    MediaPlayer {
        id: mediaplayer
        source: "file:///Volumes/Macintosh%20HD//Users/sergei/Pictures/AC34-2012--07-13/VID_20130713_123700.mp4"
        audioOutput: AudioOutput {}
        videoOutput: videoOutput
        onErrorOccurred: function (error, errorString) {
            console.log('error=', error, 'errorString=', errorString)
        }
        onPlaybackStateChanged: function (error, errorString) {
            console.log('onPlaybackStateChanged', playbackState)
        }

    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }


}
