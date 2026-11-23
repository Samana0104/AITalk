import QtQuick
import QtQuick.Controls.Basic

// 튀어나오듯 열리는 메뉴.
Menu {
    id: menu

    padding: 6
    font.pointSize: App.fontSize

    background: Rectangle {
        implicitWidth: 220
        radius: 12
        color: App.colors.surface
        border.width: 1
        border.color: App.colors.lineStrong
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
            NumberAnimation { property: "scale"; from: 0.82; to: 1; duration: 300; easing.type: Easing.OutBack; easing.overshoot: 2 }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; to: 0; duration: 110 }
            NumberAnimation { property: "scale"; to: 0.92; duration: 110 }
        }
    }
}
