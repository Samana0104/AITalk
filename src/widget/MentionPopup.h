#pragma once

#include <QColor>
#include <QFrame>
#include <QList>

class QListWidget;

// 입력창에서 @ 를 치면 뜨는 멤버 목록. ↑↓ 로 고르고 Enter/Tab 으로 넣는다.
class MentionPopup : public QFrame {
    Q_OBJECT
public:
    struct Candidate {
        QString id;
        QString name;
        QColor color;
    };

    explicit MentionPopup(QWidget* parent);

    void setCandidates(const QList<Candidate>& candidates);
    bool hasCandidates() const;
    void moveSelection(int delta);
    QString currentName() const;

signals:
    void picked(const QString& name);

private:
    QListWidget* list;
};
