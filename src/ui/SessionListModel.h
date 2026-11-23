#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace AITalk
{

class SessionManager;

// 왼쪽 세션 목록. query 가 비어 있으면 최근 활동 순 세션, 아니면 검색 결과다.
// 목록을 통째로 갈지 않고 행을 옮기고·넣고·빼서 QML 쪽 이동 애니메이션이 보이게 한다.
class SessionListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString query READ Query WRITE SetQuery NOTIFY QueryChanged)

public:
    enum class Role
    {
        SESSION_ID = Qt::UserRole + 1,
        TITLE,
        PREVIEW,
        TIME_TEXT,
        IS_CURRENT
    };

    explicit SessionListModel(SessionManager* sessions, QObject* parent = nullptr);

    QString Query() const { return this->query; }
    void SetQuery(const QString& query);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void QueryChanged();

private:
    struct Row
    {
        QString key; // 세션 id (검색 결과는 "세션 id:메시지 id")
        QString sessionId;
        QString title;
        QString preview;
        qint64 updatedAt = 0;
    };

    QList<Row> BuildRows() const;
    void Rebuild();
    static QString RelativeTime(qint64 timestamp);

private:
    SessionManager* sessions;
    QString query;
    QList<Row> rows;
    QTimer rebuildTimer;
};

} // namespace AITalk
