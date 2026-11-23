import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 메시지 한 줄: 시스템 알림 또는 말풍선(프사 · 이름 · 인용 · 본문/이모티콘 · 반응).
// 새로 들어온 메시지만 튀어나온다 (ListView.onAdd 는 처음 불러온 행에는 오지 않는다).
Item {
    id: row

    required property int messageId
    required property string sender
    required property string author
    required property color authorColor
    required property string body
    required property string previewText
    required property bool isUser
    required property bool isSystem
    required property string kind
    required property string timeText
    required property string fullTime
    required property string modelLabel
    required property bool isBoosted
    required property string emoticonSource
    required property string replyAuthor
    required property string replyExcerpt
    required property var reactions

    readonly property real maxBubbleWidth: Math.max(240, width * 0.68)
    readonly property bool isEmoticon: emoticonSource.length > 0
    property bool isReady: false

    signal replyRequested(int messageId, string author, string excerpt)

    implicitHeight: isSystem ? systemNotice.height + 12 : chat.implicitHeight + 8

    Component.onCompleted: Qt.callLater(() => row.isReady = true)

    ListView.onAdd: {
        if (isSystem) {
            systemPop.restart()
        } else if (isEmoticon) {
            emoticonPop.restart()
        } else {
            bubblePop.restart()
        }
        if (!isUser && !isSystem) {
            avatar.bounce()
        }
    }

    // ---------- 시스템 알림 ----------
    Rectangle {
        id: systemNotice

        readonly property color tone: row.kind === "error" ? App.colors.danger
                                    : row.kind === "boost" ? App.colors.boost : App.colors.muted

        visible: row.isSystem
        anchors.horizontalCenter: parent.horizontalCenter
        y: 6
        width: Math.min(systemText.implicitWidth + 28, row.width - 64)
        height: systemText.implicitHeight + 10
        radius: height / 2
        color: row.kind === "error" ? App.colors.dangerSoft : row.kind === "boost" ? App.colors.boostSoft : App.colors.accentSoft

        Text {
            id: systemText
            anchors.centerIn: parent
            width: Math.min(implicitWidth, row.width - 92)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: row.isSystem ? row.body : ""
            color: systemNotice.tone
            font.pointSize: App.fontSize - 1
        }

        HoverHandler {
            id: systemHover
        }
        ToolTip.visible: systemHover.hovered
        ToolTip.text: row.fullTime
        ToolTip.delay: 600
    }

    // ---------- 말풍선 ----------
    RowLayout {
        id: chat

        visible: !row.isSystem
        x: 16
        width: row.width - 32
        y: 4
        spacing: 10
        layoutDirection: row.isUser ? Qt.RightToLeft : Qt.LeftToRight

        Avatar {
            id: avatar
            Layout.alignment: Qt.AlignTop
            visible: !row.isUser
            memberId: row.isUser || row.isSystem ? "" : row.sender
            size: 40
        }

        ColumnLayout {
            id: content
            Layout.alignment: Qt.AlignTop
            Layout.maximumWidth: row.maxBubbleWidth
            spacing: 3
            transformOrigin: row.isUser ? Item.BottomRight : Item.BottomLeft

            // 이름 · 시간 · 모델 + (마우스를 올리면) 답장 · 복사 · 반응
            RowLayout {
                Layout.alignment: row.isUser ? Qt.AlignRight : Qt.AlignLeft
                layoutDirection: row.isUser ? Qt.RightToLeft : Qt.LeftToRight
                spacing: 6

                Text {
                    visible: !row.isUser
                    text: row.author
                    color: row.authorColor
                    font.pointSize: App.fontSize - 1
                    font.weight: Font.Bold
                }

                Text {
                    text: row.timeText + (row.modelLabel.length > 0 ? " · " + row.modelLabel : "") + (row.isBoosted ? " · 🔥" : "")
                    color: App.colors.muted
                    font.pointSize: App.fontSize - 2
                }

                Row {
                    opacity: hover.hovered ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation { duration: 150 }
                    }

                    IconButton {
                        width: 22
                        height: 22
                        iconName: "reply"
                        iconSize: 13
                        iconColor: App.colors.muted
                        tip: App.Localize("message.reply")
                        onClicked: row.replyRequested(row.messageId, row.author, row.previewText)
                    }
                    IconButton {
                        width: 22
                        height: 22
                        iconName: "copy"
                        iconSize: 13
                        iconColor: App.colors.muted
                        tip: App.Localize("message.copy")
                        onClicked: App.CopyText(row.previewText)
                    }
                    AbstractButton {
                        id: reactButton
                        width: 22
                        height: 22
                        hoverEnabled: true
                        scale: pressed ? 0.8 : hovered ? 1.2 : 1
                        Behavior on scale {
                            NumberAnimation { duration: 200; easing.type: Easing.OutBack; easing.overshoot: 3 }
                        }
                        contentItem: Text {
                            text: "☺"
                            color: App.colors.muted
                            font.pointSize: App.fontSize + 1
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: null
                        onClicked: picker.openAt(reactButton)
                        ToolTip.visible: hovered
                        ToolTip.text: App.Localize("message.react")
                        ToolTip.delay: 500
                    }
                }
            }

            // 인용 답장
            Rectangle {
                Layout.alignment: row.isUser ? Qt.AlignRight : Qt.AlignLeft
                Layout.maximumWidth: row.maxBubbleWidth
                visible: row.replyAuthor.length > 0
                implicitWidth: replyText.implicitWidth + 16
                implicitHeight: replyText.implicitHeight + 6
                radius: 8
                color: App.colors.accentSoft

                Rectangle {
                    width: 3
                    height: parent.height - 6
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: App.colors.lineStrong
                }

                Text {
                    id: replyText
                    x: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, row.maxBubbleWidth - 16)
                    text: "↪ " + row.replyAuthor + ": " + row.replyExcerpt
                    elide: Text.ElideRight
                    color: App.colors.muted
                    font.pointSize: App.fontSize - 2
                }
            }

            // 본문 말풍선
            Rectangle {
                id: bubble

                Layout.alignment: row.isUser ? Qt.AlignRight : Qt.AlignLeft
                visible: !row.isEmoticon
                implicitWidth: bodyText.width + 24
                implicitHeight: bodyText.height + 16
                radius: 14
                color: row.isUser ? App.colors.accent : App.colors.surface
                border.width: row.isUser ? 0 : row.isBoosted ? 1.5 : 1
                border.color: row.isBoosted ? App.colors.boost : App.colors.line

                TextEdit {
                    id: bodyText
                    x: 12
                    y: 8
                    width: Math.min(implicitWidth, row.maxBubbleWidth - 24)
                    text: row.isSystem || row.isEmoticon ? "" : row.body
                    // AI 답은 마크다운으로, 방장 글은 쓴 그대로 보여 준다.
                    textFormat: row.isUser ? TextEdit.PlainText : TextEdit.MarkdownText
                    wrapMode: TextEdit.Wrap
                    readOnly: true
                    selectByMouse: true
                    color: row.isUser ? App.colors.accentInk : App.colors.ink
                    selectionColor: App.colors.link
                    selectedTextColor: "#ffffff"
                    font.pointSize: App.fontSize
                    onLinkActivated: link => App.OpenUrl(link)

                    HoverHandler {
                        cursorShape: bodyText.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.IBeamCursor
                    }
                }
            }

            // 이모티콘: 말풍선 없이 그림만
            Image {
                id: emoticon
                Layout.alignment: row.isUser ? Qt.AlignRight : Qt.AlignLeft
                visible: row.isEmoticon
                source: row.emoticonSource
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                sourceSize: Qt.size(100 * Screen.devicePixelRatio, 100 * Screen.devicePixelRatio)
                fillMode: Image.PreserveAspectFit
                smooth: true
                transformOrigin: Item.Bottom

                HoverHandler {
                    id: emoticonHover
                }
                scale: emoticonHover.hovered ? 1.06 : 1
                Behavior on scale {
                    NumberAnimation { duration: 260; easing.type: Easing.OutBack; easing.overshoot: 3 }
                }
            }

            // 반응
            Flow {
                Layout.alignment: row.isUser ? Qt.AlignRight : Qt.AlignLeft
                Layout.maximumWidth: row.maxBubbleWidth
                Layout.preferredWidth: childrenRect.width
                visible: row.reactions.length > 0
                spacing: 4

                Repeater {
                    model: row.reactions

                    delegate: AbstractButton {
                        id: pill

                        required property var modelData

                        implicitHeight: 24
                        implicitWidth: pillText.implicitWidth + 16
                        hoverEnabled: true
                        scale: pressed ? 0.85 : hovered ? 1.1 : 1
                        onClicked: App.React(row.messageId, modelData.emoji)

                        Behavior on scale {
                            NumberAnimation { duration: 240; easing.type: Easing.OutBack; easing.overshoot: 3 }
                        }

                        Component.onCompleted: {
                            if (row.isReady) {
                                pillPop.restart()
                            }
                        }

                        SequentialAnimation {
                            id: pillPop
                            NumberAnimation { target: pill; property: "scale"; from: 0; to: 1.35; duration: 160; easing.type: Easing.OutQuad }
                            NumberAnimation { target: pill; property: "scale"; to: 1; duration: 380; easing.type: Easing.OutElastic }
                        }

                        background: Rectangle {
                            radius: height / 2
                            color: pill.modelData.isMine ? App.colors.accentSoft : App.colors.surface
                            border.width: 1
                            border.color: pill.modelData.isMine ? App.colors.accent : App.colors.line
                        }

                        contentItem: Text {
                            id: pillText
                            text: pill.modelData.emoji + " " + pill.modelData.count
                            color: App.colors.ink
                            font.pointSize: App.fontSize - 1
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: modelData.names
                        ToolTip.delay: 300
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    HoverHandler {
        id: hover
        enabled: !row.isSystem
    }

    // 우클릭 메뉴. 왼쪽 버튼은 받지 않으니 글자 선택은 그대로 된다.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        enabled: !row.isSystem
        onClicked: messageMenu.popup()
    }

    PopMenu {
        id: messageMenu

        MenuItem {
            text: App.Localize("message.reply")
            onTriggered: row.replyRequested(row.messageId, row.author, row.previewText)
        }
        MenuItem {
            text: App.Localize("message.copy")
            onTriggered: App.CopyText(row.previewText)
        }
        MenuItem {
            text: App.Localize("message.copySelection")
            visible: bodyText.selectedText.length > 0
            height: visible ? implicitHeight : 0
            onTriggered: App.CopyText(bodyText.selectedText)
        }
        MenuItem {
            text: App.Localize("message.react")
            onTriggered: picker.openAt(row.isEmoticon ? emoticon : bubble)
        }
    }

    ReactionPicker {
        id: picker
        onPicked: emoji => App.React(row.messageId, emoji)
    }

    // ---------- 등장 애니메이션 ----------
    SequentialAnimation {
        id: bubblePop
        PropertyAction { target: content; property: "scale"; value: 0.4 }
        PropertyAction { target: content; property: "opacity"; value: 0 }
        ParallelAnimation {
            NumberAnimation { target: content; property: "opacity"; to: 1; duration: 140 }
            NumberAnimation { target: content; property: "scale"; to: 1; duration: 460; easing.type: Easing.OutBack; easing.overshoot: 2.2 }
        }
    }

    SequentialAnimation {
        id: emoticonPop
        PropertyAction { target: content; property: "scale"; value: 0 }
        PropertyAction { target: emoticon; property: "rotation"; value: -18 }
        ParallelAnimation {
            NumberAnimation { target: content; property: "scale"; to: 1; duration: 620; easing.type: Easing.OutElastic; easing.amplitude: 1.3; easing.period: 0.45 }
            NumberAnimation { target: emoticon; property: "rotation"; to: 0; duration: 700; easing.type: Easing.OutElastic; easing.amplitude: 1.5; easing.period: 0.35 }
        }
    }

    SequentialAnimation {
        id: systemPop
        PropertyAction { target: systemNotice; property: "scale"; value: 0.6 }
        PropertyAction { target: systemNotice; property: "opacity"; value: 0 }
        ParallelAnimation {
            NumberAnimation { target: systemNotice; property: "opacity"; to: 1; duration: 200 }
            NumberAnimation { target: systemNotice; property: "scale"; to: 1; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 2 }
        }
    }
}
