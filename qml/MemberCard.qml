import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 인스펙터의 멤버 카드: 프사 · 이름 · 상태 · 모델 · 호출 통계 · 마지막 오류.
Rectangle {
    id: card

    required property string memberId
    required property string name
    required property color memberColor
    required property string status
    required property string statusLabel
    required property string modelLabel
    required property string cliPath
    required property int calls
    required property real lastSeconds
    required property string lastError

    implicitHeight: layout.implicitHeight + 20
    radius: 14
    color: App.colors.surface
    border.width: 1
    border.color: hover.hovered ? card.memberColor : App.colors.line
    transform: Translate { id: lift; y: hover.hovered ? -2 : 0 }

    Behavior on border.color {
        ColorAnimation { duration: 180 }
    }

    HoverHandler {
        id: hover
    }

    // 상태가 바뀌면 프사가 한 번 튄다.
    onStatusChanged: avatar.bounce()

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Avatar {
            id: avatar
            Layout.alignment: Qt.AlignTop
            memberId: card.memberId
            size: 36
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                spacing: 6

                Text {
                    text: card.name
                    color: card.memberColor
                    font.weight: Font.Bold
                    font.pointSize: App.fontSize
                }

                StatusDot {
                    status: card.status
                }

                Text {
                    text: card.statusLabel
                    color: App.colors.muted
                    font.pointSize: App.fontSize - 2
                }
            }

            Text {
                Layout.fillWidth: true
                text: card.cliPath.length === 0 ? App.Localize("member.noCli") : card.modelLabel
                wrapMode: Text.Wrap
                color: App.colors.muted
                font.pointSize: App.fontSize - 2

                HoverHandler {
                    id: modelHover
                }
                ToolTip.visible: modelHover.hovered && card.cliPath.length > 0
                ToolTip.text: card.cliPath
            }

            Text {
                text: card.calls > 0 ? App.Localize("member.stats").arg(card.calls).arg(card.lastSeconds.toFixed(1))
                                     : App.Localize("member.noCalls")
                color: App.colors.muted
                font.pointSize: App.fontSize - 2
            }

            Text {
                Layout.fillWidth: true
                visible: card.lastError.length > 0
                text: card.lastError.substring(0, 160)
                wrapMode: Text.Wrap
                color: App.colors.danger
                font.pointSize: App.fontSize - 2
            }
        }
    }
}
