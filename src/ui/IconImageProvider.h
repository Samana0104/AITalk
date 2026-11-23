#pragma once

#include <QQuickImageProvider>

namespace AITalk
{

// image://icon/<이름>/<RRGGBB> -> res/icons/<이름>.svg 의 currentColor 를 그 색으로 칠한 그림.
// URL 에서 '#' 은 조각 구분자라 색은 '#' 없이 받는다.
class IconImageProvider : public QQuickImageProvider
{
public:
    IconImageProvider();

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

} // namespace AITalk
