#pragma once

#include <QFrame>

// 칸이 모자라면 "…" 로 줄이는 한 줄 라벨. 전체 문장은 툴팁으로.
class ElidedLabel : public QFrame {
    Q_OBJECT
public:
    explicit ElidedLabel(const QString& text = {}, QWidget* parent = nullptr);

    void setText(const QString& text);
    QString text() const { return m_text; }
    void setElideMode(Qt::TextElideMode mode);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void doubleClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QString m_text;
    Qt::TextElideMode m_mode = Qt::ElideRight;
};
