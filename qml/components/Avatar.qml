import QtQuick

// 멤버 SD 프사 (AvatarImageProvider). bounce() 로 통통 튀게 할 수 있다.
Image {
    id: avatar

    property string memberId
    property int size: 40

    function bounce() {
        bounceAnimation.restart()
    }

    width: size
    height: size
    sourceSize: Qt.size(size * Screen.devicePixelRatio, size * Screen.devicePixelRatio)
    source: memberId.length > 0 ? "image://avatar/" + memberId : ""
    smooth: true
    transformOrigin: Item.Bottom

    SequentialAnimation {
        id: bounceAnimation
        NumberAnimation { target: avatar; property: "scale"; to: 1.18; duration: 110; easing.type: Easing.OutQuad }
        NumberAnimation { target: avatar; property: "scale"; to: 1.0; duration: 420; easing.type: Easing.OutElastic; easing.amplitude: 1.2; easing.period: 0.4 }
    }
}
