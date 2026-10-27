import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Fusion

Dialog {
    id: surfParamsDialog
    title: "Detect surfing"
    modal: true
    anchors.centerIn: Overlay.overlay

    required property var raceModel

    standardButtons: Dialog.Ok | Dialog.Cancel

    function load(params) {
        enterRatio.text = params.enterRatio.toFixed(2)
        exitRatio.text = params.exitRatio.toFixed(2)
        minTwa.value = params.minTwaDeg
        minDuration.value = params.minDurationSec
        mergeGap.value = params.mergeGapSec
        maxPerRace.value = params.maxPerRace
        minSpacing.value = params.minSpacingSec
        preRoll.value = params.preRollSec
        postRoll.value = params.postRollSec
    }

    onOpened: {
        load(raceModel.surfParams())
    }

    onAccepted: {
        raceModel.detectSurfing({
            "enterRatio": parseFloat(enterRatio.text),
            "exitRatio": parseFloat(exitRatio.text),
            "minTwaDeg": minTwa.value,
            "minDurationSec": minDuration.value,
            "mergeGapSec": mergeGap.value,
            "maxPerRace": maxPerRace.value,
            "minSpacingSec": minSpacing.value,
            "preRollSec": preRoll.value,
            "postRollSec": postRoll.value
        })
    }

    GridLayout {
        columns: 2

        Label { text: "Start surf when speed/polar ≥" }
        TextField {
            id: enterRatio
            validator: DoubleValidator { bottom: 1.0; top: 5.0; decimals: 2 }
        }

        Label { text: "End surf when speed/polar <" }
        TextField {
            id: exitRatio
            validator: DoubleValidator { bottom: 0.5; top: 5.0; decimals: 2 }
        }

        Label { text: "Min |TWA| (deg)" }
        SpinBox { id: minTwa; from: 0; to: 180; editable: true }

        Label { text: "Min surf duration (s)" }
        SpinBox { id: minDuration; from: 1; to: 120; editable: true }

        Label { text: "Merge surfs closer than (s)" }
        SpinBox { id: mergeGap; from: 0; to: 300; editable: true }

        Label { text: "Max surfs per race" }
        SpinBox { id: maxPerRace; from: 1; to: 100; editable: true }

        Label { text: "Min time between surfs (s)" }
        SpinBox { id: minSpacing; from: 0; to: 3600; stepSize: 10; editable: true }

        Label { text: "Seconds before surf" }
        SpinBox { id: preRoll; from: 0; to: 60; editable: true }

        Label { text: "Seconds after surf" }
        SpinBox { id: postRoll; from: 0; to: 60; editable: true }

        Button {
            text: "Defaults"
            onClicked: load(raceModel.defaultSurfParams())
        }
    }
}
