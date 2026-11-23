import QtQuick

// 읽는 중·입력 중인 멤버들. 점 세 개가 파도처럼 튄다. 아무도 없으면 접힌다.
Item {
    id: indicator

    readonly property bool hasBusy: {
        for (let i = 0; i < pills.count; ++i) {
            const pill = pills.itemAt(i)
            if (pill && pill.isBusy) {
                return true
            }
        }
        return false
    }

    implicitHeight: hasBusy ? 30 : 0
    clip: true

    Behavior on implicitHeight {
        NumberAnimation { duration: 260; easing.type: Easing.OutBack; easing.overshoot: 1.5 }
    }

    Row {
        x: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Repeater {
            id: pills
            model: App.memberModel

            delegate: Rectangle {
                id: pill

                required property string memberId
                required property string name
                required property string status
                required property string statusLabel

                readonly property bool isBusy: status === "reading" || status === "typing"

                visible: isBusy
                height: 24
                width: pillRow.implicitWidth + 16
                radius: 12
                color: App.colors.accentSoft
                scale: isBusy ? 1 : 0

                Behavior on scale {
                    NumberAnimation { duration: 340; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
                }

                Row {
                    id: pillRow
                    anchors.centerIn: parent
                    spacing: 5

                    Avatar {
                        anchors.verticalCenter: parent.verticalCenter
                        memberId: pill.memberId
                        size: 16
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: pill.name + " " + pill.statusLabel
                        color: App.colors.muted
                        font.pointSize: App.fontSize - 2
                    }

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Repeater {
                            model: 3

                            delegate: Rectangle {
                                id: dot

                                required property int index

                                width: 4
                                height: 4
                                radius: 2
                                color: pill.status === "typing" ? "#3b82f6" : "#f59e0b"
                                transform: Translate { id: jump }

                                SequentialAnimation {
                                    running: pill.isBusy
                                    loops: Animation.Infinite
                                    PauseAnimation { duration: dot.index * 120 }
                                    NumberAnimation { target: jump; property: "y"; to: -4; duration: 220; easing.type: Easing.OutQuad }
                                    NumberAnimation { target: jump; property: "y"; to: 0; duration: 220; easing.type: Easing.InQuad }
                                    PauseAnimation { duration: (2 - dot.index) * 120 + 200 }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
