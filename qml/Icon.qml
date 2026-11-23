import QtQuick

// res/icons/<name>.svg 를 tint 색으로 칠해 그린다 (IconImageProvider).
Image {
    id: icon

    property string name
    property int size: 16
    property color tint: App.colors.inkSoft

    width: size
    height: size
    sourceSize: Qt.size(size * Screen.devicePixelRatio, size * Screen.devicePixelRatio)
    source: name.length > 0 ? "image://icon/" + name + "/" + tint.toString().substring(1) : ""
    fillMode: Image.PreserveAspectFit
    smooth: true
}
