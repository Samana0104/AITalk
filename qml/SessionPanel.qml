import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 왼쪽: 세션 목록 · 메시지 검색 · 환경 진단.
Rectangle {
    id: panel

    property string menuSessionId

    color: App.colors.sidebar

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 12
        anchors.bottomMargin: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: App.Localize("sessions.title")
                color: App.colors.ink
                font.pointSize: App.fontSize + 1
                font.weight: Font.Bold
            }

            Item {
                Layout.fillWidth: true
            }

            ChipButton {
                iconName: "plus"
                text: App.Localize("sessions.new")
                onClicked: {
                    search.text = ""
                    App.CreateSession()
                }
            }
        }

        TextField {
            id: search
            Layout.fillWidth: true
            placeholderText: App.Localize("sessions.search")
            leftPadding: 32
            color: App.colors.ink
            selectByMouse: true
            onTextChanged: App.sessionModel.query = text

            background: Rectangle {
                implicitHeight: 34
                radius: 10
                color: App.colors.surface
                border.width: search.activeFocus ? 2 : 1
                border.color: search.activeFocus ? App.colors.accent : App.colors.line
                Behavior on border.color {
                    ColorAnimation { duration: 150 }
                }
            }

            Icon {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                size: 14
                tint: App.colors.muted
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: App.sessionModel
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            // 새 세션은 옆에서 튀어 들어오고, 활동이 생긴 세션은 위로 미끄러져 올라간다.
            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
                    NumberAnimation { property: "x"; from: -40; to: 0; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 1.8 }
                }
            }
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 0; duration: 200 }
                    NumberAnimation { property: "scale"; to: 0.7; duration: 220; easing.type: Easing.InBack }
                }
            }
            move: Transition {
                NumberAnimation { properties: "x,y"; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 1.2 }
            }
            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 320; easing.type: Easing.OutCubic }
            }

            delegate: AbstractButton {
                id: item

                required property string sessionId
                required property string title
                required property string preview
                required property string timeText
                required property bool isCurrent

                width: ListView.view.width
                height: 58
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                scale: pressed ? 0.97 : 1

                Behavior on scale {
                    NumberAnimation { duration: 200; easing.type: Easing.OutBack; easing.overshoot: 3 }
                }

                onClicked: App.SelectSession(sessionId)

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        panel.menuSessionId = item.sessionId
                        sessionMenu.popup()
                    }
                }

                background: Rectangle {
                    radius: 10
                    color: item.isCurrent ? App.colors.surface : item.hovered ? App.colors.hover : "transparent"
                    border.width: item.isCurrent ? 1 : 0
                    border.color: App.colors.line
                    Behavior on color {
                        ColorAnimation { duration: 160 }
                    }

                    // 선택된 세션 왼쪽의 작은 막대가 쭉 늘어난다.
                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: item.isCurrent ? parent.height - 22 : 0
                        radius: 2
                        color: App.colors.accent
                        Behavior on height {
                            NumberAnimation { duration: 380; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: 2

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10
                        spacing: 6

                        Text {
                            Layout.fillWidth: true
                            text: item.title
                            elide: Text.ElideRight
                            color: App.colors.ink
                            font.weight: Font.Bold
                            font.pointSize: App.fontSize
                        }

                        Text {
                            text: item.timeText
                            color: App.colors.muted
                            font.pointSize: App.fontSize - 2
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10
                        text: item.preview
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        color: App.colors.muted
                        font.pointSize: App.fontSize - 1
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: list.count === 0 && search.text.trim().length > 0
                text: App.Localize("sessions.noResults")
                color: App.colors.muted
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            ChipButton {
                text: App.Localize("sessions.doctor")
                onClicked: App.OpenDoctor()
            }

            ChipButton {
                text: App.Localize("sessions.rescan")
                tip: App.Localize("sessions.rescanTip")
                onClicked: App.RescanCli()
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    PopMenu {
        id: sessionMenu

        MenuItem {
            text: App.Localize("sessions.rename")
            onTriggered: {
                renameDialog.inputText = App.SessionTitle(panel.menuSessionId)
                renameDialog.open()
            }
        }
        MenuItem {
            text: App.Localize("common.exportMarkdown")
            onTriggered: App.ExportSession(panel.menuSessionId)
        }
        MenuSeparator {}
        MenuItem {
            text: App.Localize("common.delete")
            onTriggered: {
                deleteDialog.message = App.Localize("sessions.deleteConfirm").arg(App.SessionTitle(panel.menuSessionId))
                deleteDialog.open()
            }
        }
    }

    GameDialog {
        id: renameDialog
        title: App.Localize("sessions.rename")
        message: App.Localize("sessions.nameLabel")
        isInput: true
        onConfirmed: App.RenameSession(panel.menuSessionId, inputText)
    }

    GameDialog {
        id: deleteDialog
        title: App.Localize("sessions.deleteTitle")
        isDanger: true
        confirmText: App.Localize("common.delete")
        onConfirmed: App.DeleteSession(panel.menuSessionId)
    }
}
