import QtQuick
import QtQuick.Layouts

// 프레임 없는 창의 제목 줄: 끌어서 이동, 두 번 눌러 최대화, 설정·최소화·최대화·닫기.
Rectangle {
    id: bar

    required property var window
    readonly property bool isMaximized: window.visibility === Window.Maximized

    function toggleMaximize() {
        isMaximized ? window.showNormal() : window.showMaximized()
    }

    implicitHeight: 38
    color: App.colors.sidebar

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: App.colors.line
    }

    DragHandler {
        target: null
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: {
            if (active) {
                bar.window.startSystemMove()
            }
        }
    }

    TapHandler {
        onDoubleTapped: bar.toggleMaximize()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        spacing: 8

        Image {
            id: logo
            source: "qrc:/icons/app.svg"
            sourceSize: Qt.size(16 * Screen.devicePixelRatio, 16 * Screen.devicePixelRatio)
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16

            // 앱을 켜면 로고가 한 바퀴 돈다.
            RotationAnimation on rotation {
                from: -360
                to: 0
                duration: 900
                easing.type: Easing.OutBack
            }
        }

        Text {
            text: "AITalk"
            color: App.colors.inkSoft
            font.pointSize: App.fontSize - 1
            font.weight: Font.Bold
        }

        Item {
            Layout.fillWidth: true
        }

        IconButton {
            iconName: "settings"
            iconSize: 15
            tip: App.Localize("title.settings")
            implicitWidth: 46
            implicitHeight: 38
            radius: 0
            onClicked: App.OpenSettings()
        }

        IconButton {
            iconName: "minimize"
            iconSize: 10
            tip: App.Localize("title.minimize")
            implicitWidth: 46
            implicitHeight: 38
            radius: 0
            isBouncy: false
            onClicked: bar.window.showMinimized()
        }

        IconButton {
            iconName: bar.isMaximized ? "restore" : "maximize"
            iconSize: 10
            tip: bar.isMaximized ? App.Localize("title.restore") : App.Localize("title.maximize")
            implicitWidth: 46
            implicitHeight: 38
            radius: 0
            isBouncy: false
            onClicked: bar.toggleMaximize()
        }

        IconButton {
            id: closeButton
            iconName: "close"
            iconSize: 10
            iconColor: hovered ? "#ffffff" : App.colors.inkSoft
            hoverColor: "#e81123"
            tip: App.Localize("title.close")
            implicitWidth: 46
            implicitHeight: 38
            radius: 0
            isBouncy: false
            onClicked: bar.window.close()
        }
    }
}
