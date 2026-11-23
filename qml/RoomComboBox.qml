import QtQuick
import QtQuick.Controls.Basic

// 헤더의 알약 모양 선택 상자 (대화 속도 · 진심모드). options: [{ value, text }]
ComboBox {
    id: combo

    property var options: []
    property string selectedValue
    property string tip

    signal picked(string value)

    model: options
    textRole: "text"
    valueRole: "value"
    currentIndex: Math.max(0, options.findIndex(option => option.value === selectedValue))
    implicitHeight: 28
    implicitWidth: widest.width + 46
    font.pointSize: App.fontSize - 1
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    scale: pressed ? 0.94 : 1

    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
    }

    onActivated: index => {
        picked(options[index].value)
        // 고르면 ComboBox 가 currentIndex 를 직접 바꿔 바인딩이 끊긴다. 세션을 바꿔도 따라가게 다시 묶는다.
        currentIndex = Qt.binding(() => Math.max(0, options.findIndex(option => option.value === selectedValue)))
    }

    // 가장 긴 항목에 맞춰 폭을 잡는다.
    TextMetrics {
        id: widest
        font: combo.font
        text: {
            let longest = ""
            for (const option of combo.options) {
                if (option.text.length > longest.length) {
                    longest = option.text
                }
            }
            return longest
        }
    }

    background: Rectangle {
        radius: height / 2
        color: combo.hovered ? App.colors.hover : App.colors.surface
        border.width: 1
        border.color: combo.popup.visible ? App.colors.accent : App.colors.lineStrong
    }

    contentItem: Text {
        leftPadding: 12
        text: combo.displayText
        font: combo.font
        color: App.colors.ink
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Text {
        x: combo.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        text: "▾"
        color: App.colors.muted
        rotation: combo.popup.visible ? 180 : 0
        Behavior on rotation {
            NumberAnimation { duration: 260; easing.type: Easing.OutBack }
        }
    }

    popup.background: Rectangle {
        radius: 10
        color: App.colors.surface
        border.width: 1
        border.color: App.colors.lineStrong
    }
    popup.enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
            NumberAnimation { property: "scale"; from: 0.85; to: 1; duration: 280; easing.type: Easing.OutBack; easing.overshoot: 2 }
        }
    }

    ToolTip.visible: hovered && !popup.visible && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 500
}
