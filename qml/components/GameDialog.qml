import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 가운데에서 통 튀어나오는 확인 창. isInput 이면 한 줄 입력칸이 붙는다.
Popup {
    id: dialog

    property string title
    property string message
    property bool isInput: false
    property bool isDanger: false
    property alias inputText: input.text
    property string confirmText: App.Localize("common.ok")

    signal confirmed()

    function confirm() {
        if (isInput && input.text.trim().length === 0) {
            shakeAnimation.restart()
            return
        }
        confirmed()
        close()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(380, parent ? parent.width - 48 : 380)
    modal: true
    focus: true
    padding: 22
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: {
        if (isInput) {
            input.forceActiveFocus()
            input.selectAll()
        }
    }

    Overlay.modal: Rectangle {
        color: "#66000000"
        Behavior on opacity {
            NumberAnimation { duration: 160 }
        }
    }

    background: Rectangle {
        radius: 18
        color: App.colors.surface
        border.width: 1
        border.color: App.colors.lineStrong
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 }
            NumberAnimation { property: "scale"; from: 0.7; to: 1; duration: 420; easing.type: Easing.OutBack; easing.overshoot: 2.2 }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; to: 0; duration: 130 }
            NumberAnimation { property: "scale"; to: 0.85; duration: 130; easing.type: Easing.InQuad }
        }
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 14

        Text {
            Layout.fillWidth: true
            text: dialog.title
            color: App.colors.ink
            font.pointSize: App.fontSize + 2
            font.weight: Font.Bold
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            visible: dialog.message.length > 0
            text: dialog.message
            color: App.colors.muted
            wrapMode: Text.Wrap
            font.pointSize: App.fontSize
        }

        TextField {
            id: input
            Layout.fillWidth: true
            visible: dialog.isInput
            font.pointSize: App.fontSize
            color: App.colors.ink
            selectByMouse: true
            onAccepted: dialog.confirm()
            background: Rectangle {
                implicitHeight: 36
                radius: 10
                color: App.colors.page
                border.width: input.activeFocus ? 2 : 1
                border.color: input.activeFocus ? App.colors.accent : App.colors.lineStrong
            }
            transform: Translate { id: shake }

            SequentialAnimation {
                id: shakeAnimation
                loops: 2
                NumberAnimation { target: shake; property: "x"; to: -8; duration: 50 }
                NumberAnimation { target: shake; property: "x"; to: 8; duration: 70 }
                NumberAnimation { target: shake; property: "x"; to: 0; duration: 50 }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            ChipButton {
                text: App.Localize("common.cancel")
                implicitHeight: 32
                onClicked: dialog.close()
            }

            ChipButton {
                text: dialog.confirmText
                implicitHeight: 32
                isActive: true
                activeColor: dialog.isDanger ? App.colors.danger : App.colors.accent
                activeInk: dialog.isDanger ? "#ffffff" : App.colors.accentInk
                onClicked: dialog.confirm()
            }
        }
    }
}
