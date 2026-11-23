import QtQuick
import QtQuick.Controls.Basic

// 아이콘만 있는 버튼. 누르면 쏙 들어갔다가 튀어나온다.
AbstractButton {
    id: control

    property string iconName
    property int iconSize: 16
    property color iconColor: App.colors.inkSoft
    property color hoverColor: App.colors.hover
    property real radius: 8
    property string tip
    property bool isBouncy: true

    implicitWidth: 32
    implicitHeight: 32
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    scale: !isBouncy ? 1 : pressed ? 0.86 : hovered ? 1.08 : 1

    Behavior on scale {
        NumberAnimation { duration: 220; easing.type: Easing.OutBack; easing.overshoot: 2.2 }
    }

    background: Rectangle {
        radius: control.radius
        color: control.hovered || control.checked ? control.hoverColor : "transparent"
        Behavior on color {
            ColorAnimation { duration: 140 }
        }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            tint: control.enabled ? control.iconColor : App.colors.muted
        }
    }

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 500
}
