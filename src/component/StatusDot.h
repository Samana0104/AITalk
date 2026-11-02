#pragma once

#include <QWidget>

// 멤버 상태 점: idle(초록) reading(주황) typing(파랑) offline(빨강) disabled/missing(회색 테두리)
class StatusDot : public QWidget {
    Q_OBJECT
public:
    explicit StatusDot(int size = 8, QWidget* parent = nullptr);

    void setStatus(const QString& status);
    QString status() const { return this->currentStatus; }

    static QColor colorFor(const QString& status);
    static QString labelFor(const QString& status);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int size;
    QString currentStatus = QStringLiteral("idle");
};
