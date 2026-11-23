import QtQuick

// 패널 사이 1px 경계선. 끌면 dragged(가로 이동량)를 보낸다.
Item {
    id: handle

    readonly property bool isDragging: area.pressed

    signal dragged(real delta)

    implicitWidth: 1

    Rectangle {
        anchors.fill: parent
        color: area.containsMouse || area.pressed ? App.colors.accent : App.colors.line
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    MouseArea {
        id: area

        property real pressX: 0

        anchors.fill: parent
        anchors.leftMargin: -3
        anchors.rightMargin: -3
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        onPressed: mouse => pressX = mouse.x
        onPositionChanged: mouse => {
            if (pressed) {
                handle.dragged(mouse.x - pressX)
            }
        }
    }
}
