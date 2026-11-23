import QtQuick

// 멤버 상태 점. 읽는 중·입력 중이면 물결처럼 퍼지는 고리가 계속 뛴다.
Item {
    id: dot

    property string status: "idle"
    property int size: 8
    readonly property bool isBusy: status === "reading" || status === "typing"
    readonly property bool isHollow: status === "disabled" || status === "missing"
    readonly property color dotColor: {
        switch (status) {
        case "reading": return "#f59e0b"
        case "typing": return "#3b82f6"
        case "offline": return "#ef4444"
        case "disabled":
        case "missing":
        case "sleeping": return "#a1a1aa"
        default: return "#22c55e"
        }
    }

    width: size
    height: size

    Rectangle {
        id: ripple
        anchors.centerIn: parent
        width: dot.size
        height: dot.size
        radius: width / 2
        color: dot.dotColor
        opacity: 0
        visible: dot.isBusy
    }

    ParallelAnimation {
        running: dot.isBusy
        loops: Animation.Infinite
        NumberAnimation { target: ripple; property: "scale"; from: 1; to: 2.8; duration: 1100; easing.type: Easing.OutCubic }
        NumberAnimation { target: ripple; property: "opacity"; from: 0.55; to: 0; duration: 1100; easing.type: Easing.OutCubic }
    }

    Rectangle {
        id: core
        anchors.fill: parent
        radius: width / 2
        color: dot.isHollow ? "transparent" : dot.dotColor
        border.width: dot.isHollow ? 1.2 : 0
        border.color: dot.dotColor
        Behavior on color {
            ColorAnimation { duration: 260 }
        }
    }

    // 상태가 바뀌면 한 번 톡 튄다.
    onStatusChanged: popAnimation.restart()

    SequentialAnimation {
        id: popAnimation
        NumberAnimation { target: core; property: "scale"; to: 1.7; duration: 120; easing.type: Easing.OutQuad }
        NumberAnimation { target: core; property: "scale"; to: 1; duration: 380; easing.type: Easing.OutElastic }
    }
}
