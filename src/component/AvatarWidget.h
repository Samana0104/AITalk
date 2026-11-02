#pragma once

#include <QColor>
#include <QPixmap>
#include <QWidget>

// 둥근 사각형 프로필 (CodePet 채팅 아바타 스타일).
// res/avatars/<id>.png (SD 캐릭터)가 있으면 파스텔 배경 위에 그리고, 없으면 색 배경에 이니셜.
class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(int size = 36, QWidget* parent = nullptr);

    void setIdentity(const QString& id, const QString& label, const QColor& color);
    QSize sizeHint() const override;

    // 멤버 프사를 size×size 로 (크기·배율별 캐시). 메뉴·목록 아이콘에도 쓴다.
    static QPixmap pixmapFor(const QString& memberId, int size, qreal dpr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int size;
    QString id;
    QString label;
    QColor color;
};
