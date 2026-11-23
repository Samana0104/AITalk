import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 채팅 영역 위쪽 바.
//  1줄: 방 제목(더블클릭으로 이름 변경) · 상태 · 참가자 칩 · 오른쪽 패널 토글
//  2줄: 방 켜기 · 대화 속도 · 진심모드 · 내보내기
Rectangle {
    id: header

    property bool isInspectorVisible: true
    property string menuMemberId
    property var menuInfo: ({})

    signal inspectorToggled()

    implicitHeight: column.implicitHeight + 16
    color: App.colors.page

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: App.colors.line
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 10
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Text {
                    id: titleText
                    anchors.fill: parent
                    visible: !titleEdit.visible
                    verticalAlignment: Text.AlignVCenter
                    text: App.roomTitle
                    elide: Text.ElideRight
                    color: App.colors.ink
                    font.pointSize: App.fontSize + 3
                    font.weight: Font.Bold

                    TapHandler {
                        onDoubleTapped: {
                            titleEdit.text = App.roomTitle
                            titleEdit.visible = true
                            titleEdit.forceActiveFocus()
                            titleEdit.selectAll()
                        }
                    }

                    HoverHandler {
                        id: titleHover
                    }

                    ToolTip.visible: titleHover.hovered
                    ToolTip.text: App.Localize("room.renameTip")
                    ToolTip.delay: 700
                }

                TextField {
                    id: titleEdit
                    anchors.fill: parent
                    visible: false
                    font.pointSize: App.fontSize + 2
                    font.weight: Font.Bold
                    color: App.colors.ink
                    selectByMouse: true

                    function commit() {
                        if (!visible) {
                            return
                        }
                        visible = false
                        if (text.trim().length > 0 && text !== App.roomTitle) {
                            App.RenameRoom(text)
                        }
                    }

                    onAccepted: commit()
                    onActiveFocusChanged: {
                        if (!activeFocus) {
                            commit()
                        }
                    }
                    Keys.onEscapePressed: visible = false

                    background: Rectangle {
                        radius: 8
                        color: App.colors.surface
                        border.width: 2
                        border.color: App.colors.accent
                    }
                }
            }

            // 방 상태: 대화 중이면 초록 불이 숨 쉬듯 깜빡인다.
            Rectangle {
                implicitHeight: 22
                implicitWidth: stateRow.implicitWidth + 16
                radius: 11
                color: App.isRunning ? "#1f22c55e" : App.colors.accentSoft

                Row {
                    id: stateRow
                    anchors.centerIn: parent
                    spacing: 5

                    Rectangle {
                        id: stateLamp
                        anchors.verticalCenter: parent.verticalCenter
                        width: 7
                        height: 7
                        radius: 4
                        color: App.isRunning ? "#22c55e" : "#a1a1aa"

                        SequentialAnimation on opacity {
                            running: App.isRunning
                            loops: Animation.Infinite
                            onStopped: stateLamp.opacity = 1
                            NumberAnimation { to: 0.25; duration: 700; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: App.isRunning ? App.Localize("room.stateRunning")
                                            : App.isSleeping ? App.Localize("room.stateSleeping") : App.Localize("room.stateOff")
                        color: App.isRunning ? "#16a34a" : App.colors.muted
                        font.pointSize: App.fontSize - 2
                        font.weight: Font.DemiBold
                    }
                }
            }

            Repeater {
                model: App.memberModel

                delegate: AgentChip {
                    id: agentChip
                    onClicked: {
                        header.menuMemberId = memberId
                        header.menuInfo = App.AgentInfo(memberId)
                        agentMenu.popup(agentChip, 0, agentChip.height + 4)
                    }
                }
            }

            IconButton {
                iconName: "sidebar-right"
                checkable: true
                checked: header.isInspectorVisible
                tip: App.Localize("room.inspectorTip")
                onClicked: header.inspectorToggled()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ChipButton {
                id: power
                iconName: "power"
                isActive: App.isRunning
                activeColor: "#16a34a"
                activeInk: "#ffffff"
                text: App.isRunning ? App.Localize("room.powerOnState") : App.Localize("room.powerOn")
                onClicked: App.SetRunning(!App.isRunning)

                // 켜져 있는 동안 버튼 둘레로 빛이 번진다.
                Rectangle {
                    id: glow
                    anchors.centerIn: parent
                    width: parent.width
                    height: parent.height
                    radius: height / 2
                    color: "transparent"
                    border.width: 2
                    border.color: "#22c55e"
                    opacity: 0
                    visible: App.isRunning
                    z: -1
                }

                ParallelAnimation {
                    running: App.isRunning
                    loops: Animation.Infinite
                    NumberAnimation { target: glow; property: "scale"; from: 1; to: 1.25; duration: 1400; easing.type: Easing.OutCubic }
                    NumberAnimation { target: glow; property: "opacity"; from: 0.7; to: 0; duration: 1400; easing.type: Easing.OutCubic }
                }
            }

            RoomComboBox {
                tip: App.Localize("room.speedTip")
                selectedValue: App.roomSpeed
                options: [
                    { value: "slow", text: App.Localize("speed.slow") },
                    { value: "normal", text: App.Localize("room.speedNormal") },
                    { value: "fast", text: App.Localize("speed.fast") }
                ]
                onPicked: value => App.SetSpeed(value)
            }

            RoomComboBox {
                tip: App.Localize("room.boostTip")
                selectedValue: App.roomBoostMode
                options: [
                    { value: "auto", text: App.Localize("room.boostAuto") },
                    { value: "manual", text: App.Localize("room.boostManual") },
                    { value: "off", text: App.Localize("room.boostOff") }
                ]
                onPicked: value => App.SetBoostMode(value)
            }

            ChipButton {
                iconName: "export"
                text: App.Localize("room.export")
                tip: App.Localize("room.exportTip")
                onClicked: App.ExportSession("")
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    PopMenu {
        id: agentMenu

        MenuItem {
            text: header.menuInfo.joinText || ""
            checkable: true
            checked: header.menuInfo.isEnabled || false
            enabled: header.menuInfo.isAvailable || false
            onTriggered: App.SetMemberEnabled(header.menuMemberId, checked)
        }
        MenuSeparator {}
        MenuItem {
            text: header.menuInfo.modelText || ""
            enabled: false
        }
        MenuItem {
            text: header.menuInfo.boostText || ""
            enabled: false
            visible: text.length > 0
            height: visible ? implicitHeight : 0
        }
        MenuItem {
            text: header.menuInfo.cliText || ""
            enabled: false
        }
        MenuItem {
            text: header.menuInfo.errorText || ""
            enabled: false
            visible: text.length > 0
            height: visible ? implicitHeight : 0
        }
        MenuSeparator {}
        MenuItem {
            text: App.Localize("agentMenu.settings")
            onTriggered: App.OpenSettings(header.menuMemberId)
        }
    }
}
