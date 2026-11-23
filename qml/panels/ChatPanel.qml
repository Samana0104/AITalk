import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 가운데: 방 헤더 · 메시지 목록 · 입력 중 표시 · 입력창.
Rectangle {
    id: panel

    property bool isInspectorVisible: true

    signal inspectorToggled()

    color: App.colors.page

    // 세션을 바꾸면 대화 내용이 아래에서 스윽 올라온다.
    Connections {
        target: App
        function onSessionChanged() {
            composer.clearReply()
            composer.focusInput()
            switchAnimation.restart()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RoomHeader {
            Layout.fillWidth: true
            isInspectorVisible: panel.isInspectorVisible
            onInspectorToggled: panel.inspectorToggled()
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageList

                anchors.fill: parent
                // 0번 행이 가장 최근 메시지. 아래에서 위로 쌓으면 새 메시지가 오면 자연히 맨 아래에 붙는다.
                verticalLayoutDirection: ListView.BottomToTop
                model: App.messageModel
                clip: true
                spacing: 2
                topMargin: 12
                bottomMargin: 12
                boundsBehavior: Flickable.StopAtBounds
                cacheBuffer: 1200
                transform: Translate { id: listShift }
                ScrollBar.vertical: ScrollBar {}

                delegate: MessageDelegate {
                    width: messageList.width
                    onReplyRequested: (messageId, author, excerpt) => composer.setReply(messageId, author, excerpt)
                }

                addDisplaced: Transition {
                    NumberAnimation { properties: "y"; duration: 260; easing.type: Easing.OutCubic }
                }
            }

            // 위로 올려 보고 있을 때 맨 아래로 돌아가는 버튼.
            AbstractButton {
                id: jumpButton

                readonly property bool isShown: messageList.count > 0 && !messageList.atYEnd

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 16
                width: 38
                height: 38
                hoverEnabled: true
                scale: isShown ? (pressed ? 0.85 : hovered ? 1.1 : 1) : 0
                visible: scale > 0.01
                onClicked: messageList.positionViewAtBeginning()

                Behavior on scale {
                    NumberAnimation { duration: 300; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
                }

                background: Rectangle {
                    radius: width / 2
                    color: App.colors.accent
                }

                contentItem: Text {
                    text: "↓"
                    color: App.colors.accentInk
                    font.pointSize: App.fontSize + 3
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        TypingIndicator {
            Layout.fillWidth: true
        }

        Composer {
            id: composer
            Layout.fillWidth: true
            onSent: messageList.positionViewAtBeginning()
        }
    }

    ParallelAnimation {
        id: switchAnimation
        NumberAnimation { target: messageList; property: "opacity"; from: 0; to: 1; duration: 260 }
        NumberAnimation { target: listShift; property: "y"; from: 40; to: 0; duration: 480; easing.type: Easing.OutBack; easing.overshoot: 1.4 }
    }
}
