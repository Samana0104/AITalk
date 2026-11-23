import QtCore
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 프레임 없는 메인 창.
//  ┌──────────────────── TitleBar ────────────────────┐
//  │ SessionPanel │       ChatPanel        │ Inspector │
//  └──────────────────────────────────────────────────┘
ApplicationWindow {
    id: window

    readonly property bool isMaximized: visibility === Window.Maximized || visibility === Window.FullScreen
    property int sidebarWidth: 260
    property int inspectorWidth: 280
    property bool isInspectorVisible: true

    width: 1240
    height: 800
    minimumWidth: 820
    minimumHeight: 520
    visible: false
    title: "AITalk"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: App.colors.page
    font.pointSize: App.fontSize

    // Basic 스타일 컨트롤(메뉴·콤보·입력칸)이 테마 색을 따르게 한다.
    palette {
        window: App.colors.page
        windowText: App.colors.ink
        base: App.colors.surface
        alternateBase: App.colors.sidebar
        text: App.colors.ink
        button: App.colors.surface
        buttonText: App.colors.ink
        brightText: App.colors.accentInk
        highlight: App.colors.accent
        highlightedText: App.colors.accentInk
        light: App.colors.hover
        midlight: App.colors.hover
        mid: App.colors.lineStrong
        dark: App.colors.lineStrong
        shadow: App.colors.line
        placeholderText: App.colors.muted
        toolTipBase: App.colors.accent
        toolTipText: App.colors.accentInk
        link: App.colors.link
    }

    Settings {
        id: uiSettings
        location: App.uiSettingsUrl
        category: "quickWindow"

        property int windowX: -1
        property int windowY: -1
        property int windowWidth: 1240
        property int windowHeight: 800
        property bool isMaximized: false
        property int sidebarWidth: 260
        property int inspectorWidth: 280
        property bool isInspectorVisible: true
    }

    function saveUiState() {
        if (!isMaximized) {
            uiSettings.windowX = x
            uiSettings.windowY = y
            uiSettings.windowWidth = width
            uiSettings.windowHeight = height
        }
        uiSettings.isMaximized = isMaximized
        uiSettings.sidebarWidth = sidebarWidth
        uiSettings.inspectorWidth = inspectorWidth
        uiSettings.isInspectorVisible = isInspectorVisible
    }

    Component.onCompleted: {
        width = uiSettings.windowWidth
        height = uiSettings.windowHeight
        if (uiSettings.windowX >= 0 && uiSettings.windowY >= 0) {
            x = uiSettings.windowX
            y = uiSettings.windowY
        }
        sidebarWidth = uiSettings.sidebarWidth
        inspectorWidth = uiSettings.inspectorWidth
        isInspectorVisible = uiSettings.isInspectorVisible
        uiSettings.isMaximized ? showMaximized() : show()
        introAnimation.start()
    }

    onClosing: close => {
        saveUiState()
        close.accepted = App.RequestClose()
        if (!close.accepted) {
            hide()
        }
    }

    Connections {
        target: App
        function onToastRequested(text) {
            toast.show(text)
        }
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        color: App.colors.page
        border.width: window.isMaximized ? 0 : 1
        border.color: App.colors.lineStrong

        ColumnLayout {
            id: shell
            anchors.fill: parent
            anchors.margins: frame.border.width
            spacing: 0

            TitleBar {
                Layout.fillWidth: true
                window: window
            }

            RowLayout {
                id: body
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0
                transform: Translate { id: bodyShift }

                SessionPanel {
                    Layout.fillHeight: true
                    Layout.preferredWidth: window.sidebarWidth
                }

                PanelDivider {
                    Layout.fillHeight: true
                    onDragged: delta => window.sidebarWidth = Math.max(200, Math.min(420, window.sidebarWidth + delta))
                }

                ChatPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    isInspectorVisible: window.isInspectorVisible
                    onInspectorToggled: window.isInspectorVisible = !window.isInspectorVisible
                }

                PanelDivider {
                    id: inspectorHandle
                    Layout.fillHeight: true
                    visible: window.isInspectorVisible
                    onDragged: delta => window.inspectorWidth = Math.max(240, Math.min(460, window.inspectorWidth - delta))
                }

                InspectorPanel {
                    id: inspector
                    Layout.fillHeight: true
                    Layout.preferredWidth: shownWidth
                    clip: true

                    // 패널이 접히고 펴질 때 미끄러지듯 움직인다. 끄는 중에는 바로 따라온다.
                    property real shownWidth: window.isInspectorVisible ? window.inspectorWidth : 0
                    Behavior on shownWidth {
                        enabled: !inspectorHandle.isDragging
                        NumberAnimation { duration: 320; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
                    }
                }
            }
        }

        // 창 가장자리를 끌어 크기 조절 (최대화면 끔)
        Repeater {
            model: [
                { edges: Qt.LeftEdge, cursor: Qt.SizeHorCursor },
                { edges: Qt.RightEdge, cursor: Qt.SizeHorCursor },
                { edges: Qt.TopEdge, cursor: Qt.SizeVerCursor },
                { edges: Qt.BottomEdge, cursor: Qt.SizeVerCursor },
                { edges: Qt.LeftEdge | Qt.TopEdge, cursor: Qt.SizeFDiagCursor },
                { edges: Qt.RightEdge | Qt.BottomEdge, cursor: Qt.SizeFDiagCursor },
                { edges: Qt.RightEdge | Qt.TopEdge, cursor: Qt.SizeBDiagCursor },
                { edges: Qt.LeftEdge | Qt.BottomEdge, cursor: Qt.SizeBDiagCursor }
            ]

            delegate: MouseArea {
                required property var modelData
                readonly property int grip: 5
                readonly property bool isLeft: modelData.edges & Qt.LeftEdge
                readonly property bool isRight: modelData.edges & Qt.RightEdge
                readonly property bool isTop: modelData.edges & Qt.TopEdge
                readonly property bool isBottom: modelData.edges & Qt.BottomEdge

                visible: !window.isMaximized
                x: isRight ? frame.width - grip : 0
                y: isBottom ? frame.height - grip : 0
                width: isLeft || isRight ? grip : frame.width
                height: isTop || isBottom ? grip : frame.height
                z: isLeft !== isRight && isTop !== isBottom ? 11 : 10
                cursorShape: modelData.cursor
                onPressed: window.startSystemResize(modelData.edges)
            }
        }
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - height - 96
        z: 100
    }

    // 창이 처음 뜰 때 내용이 살짝 떠오르며 나타난다.
    ParallelAnimation {
        id: introAnimation
        NumberAnimation { target: shell; property: "opacity"; from: 0; to: 1; duration: 380; easing.type: Easing.OutCubic }
        NumberAnimation { target: bodyShift; property: "y"; from: 26; to: 0; duration: 620; easing.type: Easing.OutBack; easing.overshoot: 1.6 }
    }
}
