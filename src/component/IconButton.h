#pragma once

#include <QToolButton>

// res/icons/<name>.svg 를 테마 색으로 칠해 쓰는 아이콘 버튼. 테마가 바뀌면 다시 칠한다.
class IconButton : public QToolButton {
    Q_OBJECT
public:
    explicit IconButton(const QString& icon, const QString& tooltip = {}, QWidget* parent = nullptr);

    void setIconName(const QString& icon);
    void setTone(const QString& token);

private:
    void refresh();

    QString m_icon;
    QString m_tone = QStringLiteral("ink-soft");
};
