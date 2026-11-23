import QtQuick
import QtQuick.Controls.Basic

// 빠른 반응 고르기: 이모지들이 차례로 톡톡 튀어나온다.
Popup {
    id: picker

    readonly property var emojis: ["👍", "😂", "❤️", "😮", "😢", "🔥"]

    signal picked(string emoji)

    function openAt(anchorItem) {
        parent = anchorItem
        x = 0
        y = -height - 6
        open()
    }

    padding: 6
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: height / 2
        color: App.colors.surface
        border.width: 1
        border.color: App.colors.lineStrong
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: 100 }
    }

    contentItem: Row {
        spacing: 2

        Repeater {
            model: picker.emojis

            delegate: AbstractButton {
                id: emojiButton

                required property string modelData
                required property int index

                width: 34
                height: 34
                hoverEnabled: true
                scale: 0
                onClicked: {
                    picker.picked(modelData)
                    picker.close()
                }

                contentItem: Text {
                    text: emojiButton.modelData
                    font.pointSize: App.fontSize + 6
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    scale: emojiButton.pressed ? 0.8 : emojiButton.hovered ? 1.35 : 1
                    Behavior on scale {
                        NumberAnimation { duration: 220; easing.type: Easing.OutBack; easing.overshoot: 3 }
                    }
                }
                background: null

                SequentialAnimation {
                    id: popIn
                    PropertyAction { target: emojiButton; property: "scale"; value: 0 }
                    PauseAnimation { duration: emojiButton.index * 35 }
                    NumberAnimation { target: emojiButton; property: "scale"; to: 1; duration: 380; easing.type: Easing.OutBack; easing.overshoot: 3 }
                }

                Connections {
                    target: picker
                    function onAboutToShow() {
                        popIn.restart()
                    }
                }
            }
        }
    }
}
