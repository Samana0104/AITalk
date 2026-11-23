import QtQuick
import QtQuick.Controls.Basic

// 헤더의 참가자 칩: 프사 · 이름 · 상태 점. 입력 중이면 프사가 통통 뛰고, 읽는 중이면 갸웃거린다.
AbstractButton {
    id: chip

    required property string memberId
    required property string name
    required property string status
    required property string statusLabel
    required property string modelLabel
    required property bool isBoosting

    readonly property bool isActive: status !== "disabled" && status !== "missing"

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 16
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    opacity: isActive ? 1 : 0.5
    scale: pressed ? 0.9 : hovered ? 1.05 : 1

    Behavior on scale {
        NumberAnimation { duration: 220; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
    }
    Behavior on opacity {
        NumberAnimation { duration: 250 }
    }

    onStatusChanged: {
        if (status === "typing") {
            avatar.bounce()
        }
    }

    background: Rectangle {
        radius: height / 2
        color: chip.hovered ? App.colors.hover : App.colors.surface
        border.width: 1
        border.color: chip.isBoosting ? App.colors.boost : App.colors.line
    }

    contentItem: Item {
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            x: 4
            spacing: 6

            Avatar {
                id: avatar
                anchors.verticalCenter: parent.verticalCenter
                memberId: chip.memberId
                size: 22
                transform: Translate { id: hop }

                SequentialAnimation {
                    running: chip.status === "typing"
                    loops: Animation.Infinite
                    onStopped: hop.y = 0
                    NumberAnimation { target: hop; property: "y"; to: -4; duration: 180; easing.type: Easing.OutQuad }
                    NumberAnimation { target: hop; property: "y"; to: 0; duration: 260; easing.type: Easing.OutBounce }
                    PauseAnimation { duration: 120 }
                }

                SequentialAnimation {
                    running: chip.status === "reading"
                    loops: Animation.Infinite
                    onStopped: avatar.rotation = 0
                    NumberAnimation { target: avatar; property: "rotation"; to: -10; duration: 420; easing.type: Easing.InOutSine }
                    NumberAnimation { target: avatar; property: "rotation"; to: 10; duration: 840; easing.type: Easing.InOutSine }
                    NumberAnimation { target: avatar; property: "rotation"; to: 0; duration: 420; easing.type: Easing.InOutSine }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: chip.name + (chip.isBoosting ? " 🔥" : "")
                color: chip.isActive ? App.colors.ink : App.colors.muted
                font.pointSize: App.fontSize - 1
                font.weight: Font.DemiBold
            }

            StatusDot {
                anchors.verticalCenter: parent.verticalCenter
                status: chip.status
            }
        }
    }

    ToolTip.visible: hovered
    ToolTip.text: statusLabel + "\n" + modelLabel
    ToolTip.delay: 400
}
