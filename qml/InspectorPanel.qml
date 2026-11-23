import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// 오른쪽: 멤버 카드 / 멤버 개인 메모.
Rectangle {
    id: panel

    property bool isNoteDirty: false
    readonly property string noteMemberId: noteMember.currentValue || ""

    function loadNote() {
        noteEditor.text = App.ReadNote(noteMemberId)
        isNoteDirty = false
    }

    color: App.colors.sidebar

    Connections {
        target: App
        function onSessionChanged() {
            panel.loadNote()
        }
        function onNoteChanged(memberId) {
            if (memberId === panel.noteMemberId && !panel.isNoteDirty) {
                panel.loadNote()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // 탭: 아래 막대가 통통 튀며 따라간다.
        Item {
            id: tabs

            property int currentIndex: 0

            Layout.fillWidth: true
            Layout.preferredHeight: 34

            Row {
                id: tabRow
                anchors.fill: parent

                Repeater {
                    id: tabRepeater
                    model: [App.Localize("inspector.tabMembers"), App.Localize("inspector.tabNotes")]

                    delegate: AbstractButton {
                        id: tab

                        required property string modelData
                        required property int index

                        width: tabRow.width / 2
                        height: tabRow.height
                        hoverEnabled: true
                        focusPolicy: Qt.NoFocus
                        onClicked: tabs.currentIndex = index

                        contentItem: Text {
                            text: tab.modelData
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: tabs.currentIndex === tab.index ? App.colors.ink : App.colors.muted
                            font.weight: tabs.currentIndex === tab.index ? Font.Bold : Font.Normal
                            font.pointSize: App.fontSize
                            scale: tab.pressed ? 0.9 : 1
                            Behavior on scale {
                                NumberAnimation { duration: 200; easing.type: Easing.OutBack; easing.overshoot: 3 }
                            }
                        }
                        background: null
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: App.colors.line
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: tabRow.width / 2 - 24
                height: 3
                radius: 2
                color: App.colors.accent
                x: tabs.currentIndex * tabRow.width / 2 + 12
                Behavior on x {
                    SpringAnimation { spring: 4; damping: 0.28; epsilon: 0.25 }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            // ---- 멤버 ----
            ListView {
                clip: true
                spacing: 8
                topMargin: 4
                model: App.memberModel
                boundsBehavior: Flickable.StopAtBounds

                delegate: MemberCard {
                    width: ListView.view.width
                }

                // 탭을 열 때 카드가 차례로 떨어진다.
                populate: Transition {
                    id: populateTransition
                    SequentialAnimation {
                        PropertyAction { property: "opacity"; value: 0 }
                        PauseAnimation { duration: populateTransition.ViewTransition.index * 70 }
                        ParallelAnimation {
                            NumberAnimation { property: "opacity"; to: 1; duration: 200 }
                            NumberAnimation { property: "y"; from: populateTransition.ViewTransition.destination.y - 30; duration: 480; easing.type: Easing.OutBack; easing.overshoot: 2 }
                        }
                    }
                }
            }

            // ---- 메모 ----
            ColumnLayout {
                spacing: 8

                ComboBox {
                    id: noteMember
                    Layout.fillWidth: true
                    model: App.memberModel
                    textRole: "name"
                    valueRole: "memberId"
                    font.pointSize: App.fontSize
                    onActivated: panel.loadNote()
                    Component.onCompleted: panel.loadNote()
                }

                Text {
                    Layout.fillWidth: true
                    text: App.Localize("inspector.notesHint")
                    wrapMode: Text.Wrap
                    color: App.colors.muted
                    font.pointSize: App.fontSize - 1
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        id: noteEditor
                        wrapMode: TextArea.Wrap
                        placeholderText: App.Localize("inspector.notesEmpty")
                        placeholderTextColor: App.colors.muted
                        color: App.colors.ink
                        selectByMouse: true
                        font.pointSize: App.fontSize - 1
                        onTextChanged: {
                            if (activeFocus) {
                                panel.isNoteDirty = true
                            }
                        }
                        background: Rectangle {
                            radius: 10
                            color: App.colors.surface
                            border.width: noteEditor.activeFocus ? 2 : 1
                            border.color: noteEditor.activeFocus ? App.colors.accent : App.colors.line
                        }
                    }
                }

                ChipButton {
                    Layout.alignment: Qt.AlignRight
                    text: App.Localize("inspector.saveNote")
                    enabled: panel.isNoteDirty
                    isActive: panel.isNoteDirty
                    opacity: enabled ? 1 : 0.5
                    onClicked: {
                        panel.isNoteDirty = false
                        App.WriteNote(panel.noteMemberId, noteEditor.text)
                    }
                }
            }
        }
    }
}
