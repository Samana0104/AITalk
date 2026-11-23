import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 입력창: 인용 답장 미리보기 · 여러 줄 입력 · @멘션 팝업 · 보내기 버튼.
//  Enter 전송, Shift+Enter 줄바꿈. 멘션 팝업이 열려 있으면 ↑↓ 선택, Enter/Tab 넣기, Esc 닫기.
Item {
    id: composer

    property int replyTo: 0
    property string replyAuthor
    property string replyExcerpt
    property var mentionCandidates: []
    property int mentionIndex: 0
    readonly property bool isMentionOpen: mentionCandidates.length > 0 && input.activeFocus

    signal sent()

    function setReply(messageId, author, excerpt) {
        replyTo = messageId
        replyAuthor = author
        replyExcerpt = excerpt
        focusInput()
    }

    function clearReply() {
        replyTo = 0
    }

    function focusInput() {
        input.forceActiveFocus()
    }

    function send() {
        // 한글 조합 중인 마지막 글자까지 확정한 뒤 읽는다.
        Qt.inputMethod.commit()
        const text = input.text.trim()
        if (text.length === 0) {
            shakeAnimation.restart()
            return
        }
        App.SendMessage(text, replyTo)
        input.clear()
        clearReply()
        launchAnimation.restart()
        sent()
    }

    function textBeforeCursor() {
        return input.getText(0, input.cursorPosition)
    }

    function updateMention() {
        const match = /@([^\s@]*)$/.exec(textBeforeCursor())
        mentionCandidates = match ? App.MentionCandidates(match[1]) : []
        mentionIndex = 0
    }

    function acceptMention() {
        const candidate = mentionCandidates[mentionIndex]
        if (!candidate) {
            return
        }
        const before = textBeforeCursor()
        const at = before.lastIndexOf("@")
        input.remove(at, input.cursorPosition)
        input.insert(at, "@" + candidate.name + " ")
        mentionCandidates = []
    }

    implicitHeight: column.implicitHeight + 22

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 6
        spacing: 6

        // 인용 답장 미리보기: 아래에서 쏙 올라온다.
        Rectangle {
            id: replyBar
            Layout.fillWidth: true
            Layout.preferredHeight: composer.replyTo !== 0 ? 30 : 0
            clip: true
            radius: 8
            color: App.colors.accentSoft
            opacity: composer.replyTo !== 0 ? 1 : 0

            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 280; easing.type: Easing.OutBack; easing.overshoot: 2 }
            }
            Behavior on opacity {
                NumberAnimation { duration: 180 }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 4

                Text {
                    Layout.fillWidth: true
                    text: "↪ " + composer.replyAuthor + ": " + composer.replyExcerpt
                    elide: Text.ElideRight
                    color: App.colors.muted
                    font.pointSize: App.fontSize - 1
                }

                IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    iconName: "close"
                    iconSize: 9
                    tip: App.Localize("composer.clearReply")
                    onClicked: composer.clearReply()
                }
            }
        }

        Rectangle {
            id: box
            Layout.fillWidth: true
            implicitHeight: Math.max(46, Math.min(input.implicitHeight, 180) + 12)
            radius: 16
            color: App.colors.surface
            border.width: input.activeFocus ? 2 : 1
            border.color: input.activeFocus ? App.colors.accent : App.colors.lineStrong
            transform: Translate { id: shake }

            Behavior on border.color {
                ColorAnimation { duration: 180 }
            }
            Behavior on implicitHeight {
                NumberAnimation { duration: 140; easing.type: Easing.OutQuad }
            }

            ScrollView {
                id: scroll
                anchors.left: parent.left
                anchors.right: sendButton.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: 6
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                anchors.rightMargin: 4

                TextArea {
                    id: input
                    wrapMode: TextArea.Wrap
                    placeholderText: App.isRunning ? App.Localize("composer.placeholder") : App.Localize("composer.placeholderOff")
                    placeholderTextColor: App.colors.muted
                    color: App.colors.ink
                    selectByMouse: true
                    font.pointSize: App.fontSize
                    background: null
                    onTextChanged: composer.updateMention()
                    onCursorPositionChanged: composer.updateMention()

                    Keys.onPressed: event => {
                        const isEnter = event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                        if (composer.isMentionOpen) {
                            const count = composer.mentionCandidates.length
                            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                                const step = event.key === Qt.Key_Up ? -1 : 1
                                composer.mentionIndex = (composer.mentionIndex + step + count) % count
                                event.accepted = true
                                return
                            }
                            if (isEnter || event.key === Qt.Key_Tab) {
                                composer.acceptMention()
                                event.accepted = true
                                return
                            }
                            if (event.key === Qt.Key_Escape) {
                                composer.mentionCandidates = []
                                event.accepted = true
                                return
                            }
                        }
                        if (isEnter && !(event.modifiers & Qt.ShiftModifier)) {
                            composer.send()
                            event.accepted = true
                        }
                    }
                }
            }

            AbstractButton {
                id: sendButton
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 6
                width: 34
                height: 34
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                scale: pressed ? 0.82 : hovered ? 1.1 : 1
                onClicked: composer.send()

                Behavior on scale {
                    NumberAnimation { duration: 240; easing.type: Easing.OutBack; easing.overshoot: 3 }
                }

                background: Rectangle {
                    radius: 11
                    color: App.colors.accent
                }

                contentItem: Item {
                    clip: true

                    Icon {
                        id: sendIcon
                        anchors.centerIn: parent
                        name: "send"
                        size: 16
                        tint: App.colors.accentInk
                        transform: Translate { id: flight }
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: App.Localize("composer.sendTip")
                ToolTip.delay: 600
            }
        }
    }

    // 보내면 종이비행기가 오른쪽 위로 날아갔다가 아래에서 다시 들어온다.
    SequentialAnimation {
        id: launchAnimation
        ParallelAnimation {
            NumberAnimation { target: flight; property: "x"; to: 26; duration: 200; easing.type: Easing.InQuad }
            NumberAnimation { target: flight; property: "y"; to: -26; duration: 200; easing.type: Easing.InQuad }
        }
        PropertyAction { target: flight; property: "x"; value: -22 }
        PropertyAction { target: flight; property: "y"; value: 22 }
        ParallelAnimation {
            NumberAnimation { target: flight; property: "x"; to: 0; duration: 320; easing.type: Easing.OutBack; easing.overshoot: 2 }
            NumberAnimation { target: flight; property: "y"; to: 0; duration: 320; easing.type: Easing.OutBack; easing.overshoot: 2 }
        }
    }

    // 빈 글을 보내려 하면 입력창이 도리도리.
    SequentialAnimation {
        id: shakeAnimation
        loops: 2
        NumberAnimation { target: shake; property: "x"; to: -7; duration: 45 }
        NumberAnimation { target: shake; property: "x"; to: 7; duration: 70 }
        NumberAnimation { target: shake; property: "x"; to: 0; duration: 45 }
    }

    // @멘션 후보: 입력창 위로 튀어 오른다.
    Rectangle {
        id: mentionPopup

        x: column.x + 8
        y: column.y + box.y - height - 6
        z: 10
        width: 220
        height: mentionList.implicitHeight + 8
        radius: 12
        color: App.colors.surface
        border.width: 1
        border.color: App.colors.lineStrong
        visible: scale > 0.01
        scale: composer.isMentionOpen ? 1 : 0
        transformOrigin: Item.BottomLeft

        Behavior on scale {
            NumberAnimation { duration: 260; easing.type: Easing.OutBack; easing.overshoot: 2 }
        }

        Column {
            id: mentionList
            anchors.centerIn: parent
            width: parent.width - 8

            Repeater {
                model: composer.mentionCandidates

                delegate: AbstractButton {
                    id: candidate

                    required property var modelData
                    required property int index
                    readonly property bool isSelected: index === composer.mentionIndex

                    width: mentionList.width
                    height: 32
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus
                    onHoveredChanged: {
                        if (hovered) {
                            composer.mentionIndex = index
                        }
                    }
                    onClicked: {
                        composer.mentionIndex = index
                        composer.acceptMention()
                        composer.focusInput()
                    }

                    background: Rectangle {
                        radius: 8
                        color: candidate.isSelected ? App.colors.hover : "transparent"
                    }

                    contentItem: Row {
                        leftPadding: 6
                        spacing: 8

                        Avatar {
                            anchors.verticalCenter: parent.verticalCenter
                            memberId: candidate.modelData.memberId
                            size: 22
                            scale: candidate.isSelected ? 1.15 : 1
                            Behavior on scale {
                                NumberAnimation { duration: 200; easing.type: Easing.OutBack; easing.overshoot: 3 }
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "@" + candidate.modelData.name
                            color: candidate.modelData.memberColor
                            font.weight: Font.DemiBold
                            font.pointSize: App.fontSize
                        }
                    }
                }
            }
        }
    }
}
