import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtMultimedia
import sails
import QtQuick.Controls.Fusion

Rectangle {
    anchors.fill: parent
    required property var raceModel
    color: "#848282"

    GoProMediaPlayer {
        id: mediaplayer
        model: raceModel

        audioOutput: AudioOutput {}
        videoOutput: videoOutput

        // Called by the player during the playback
        onRacePathIdxChanged: function (idx) {
            model.seekToRacePathIdx(idx)
        }

        // Called by the player during the playback
        onPositionChanged: function () {
            // Update time label
            let posSt = new Date(position).toISOString().substring(14, 19)
            let durSt = new Date(duration).toISOString().substring(14, 19)

            clipPositionLabel.text = posSt + " / " + durSt
        }

        onErrorOccurred: function (error, errorString) {
            console.log('error=', error, 'errorString=', errorString)
        }

        onClipNameChanged: function (clipName) {
            clipNameLabel.text = clipName
        }

    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }

    Item {
        height: 50
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: mediaplayer.playbackState ===  MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
            onClicked: {
                switch(mediaplayer.playbackState) {
                    case MediaPlayer.PlayingState: mediaplayer.pauseClip(); break;
                    case MediaPlayer.PausedState: mediaplayer.playClip(); break;
                    case MediaPlayer.StoppedState: mediaplayer.playClip(); break;
                }
            }
        }
        Label {
            anchors.left: parent.left
            id: clipNameLabel
            text: ""
        }

        Label {
            anchors.right: parent.right
            id: clipPositionLabel
            text: ""
        }

    }



    function seekTo(idx) {
        mediaplayer.seekTo(idx)
    }

}
