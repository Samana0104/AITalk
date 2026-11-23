import QtQuick

// 화면 아래에서 튀어 오르는 알림. show(문구) 로 띄운다.
Rectangle {
    id: toast

    property real lift: 24

    function show(message) {
        label.text = message
        hideTimer.restart()
        showAnimation.restart()
    }

    width: Math.min(label.implicitWidth + 36, parent.width - 48)
    height: label.implicitHeight + 18
    radius: height / 2
    color: App.colors.accent
    opacity: 0
    scale: 0.8
    visible: opacity > 0
    transform: Translate { y: toast.lift }

    Text {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, toast.parent.width - 84)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: App.colors.accentInk
        font.pointSize: App.fontSize
        font.weight: Font.DemiBold
    }

    ParallelAnimation {
        id: showAnimation
        NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 160 }
        NumberAnimation { target: toast; property: "scale"; to: 1; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 2.4 }
        NumberAnimation { target: toast; property: "lift"; from: 24; to: 0; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 2 }
    }

    ParallelAnimation {
        id: hideAnimation
        NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 220 }
        NumberAnimation { target: toast; property: "lift"; to: -12; duration: 220; easing.type: Easing.InQuad }
        NumberAnimation { target: toast; property: "scale"; to: 0.9; duration: 220 }
    }

    Timer {
        id: hideTimer
        interval: 2600
        onTriggered: hideAnimation.restart()
    }
}
