#pragma once

#include <QQuickImageProvider>

namespace AITalk
{

// image://avatar/<멤버 id> -> 둥근 사각형 프로필 (CodePet 채팅 아바타 스타일).
// res/avatars/<id>.png (SD 캐릭터)가 있으면 파스텔 배경 위에, 없으면 색 배경에 이니셜.
class AvatarImageProvider : public QQuickImageProvider
{
public:
    AvatarImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};

} // namespace AITalk
