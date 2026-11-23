import QtQuick
import QtQuick.Controls.Basic

// 알약 모양 버튼 (아이콘 + 글자). isActive 면 진하게 칠해진다.
AbstractButton {
    id: control

    property string iconName
    property string tip
    property color activeColor: App.colors.accent
    property color activeInk: App.colors.accentInk
    property bool isActive: checked
    readonly property color inkColor: isActive ? activeInk : App.colors.ink

    implicitHeight: 28
    implicitWidth: row.implicitWidth + 24
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    font.pointSize: App.fontSize - 1
    scale: pressed ? 0.9 : hovered ? 1.04 : 1

    Behavior on scale {
        NumberAnimation { duration: 220; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
    }

    background: Rectangle {
        radius: height / 2
        color: control.isActive ? control.activeColor : control.hovered ? App.colors.hover : App.colors.surface
        border.width: 1
        border.color: control.isActive ? control.activeColor : App.colors.lineStrong
        Behavior on color {
            ColorAnimation { duration: 180 }
        }
    }

    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 6

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName.length > 0
                name: control.iconName
                size: 13
                tint: control.inkColor
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.inkColor
            }
        }
    }

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 500
}
