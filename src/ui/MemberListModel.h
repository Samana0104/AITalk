#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

class AgentHub;
class RoomEngine;

// 방 멤버(AI)의 실시간 상태 목록. 헤더 칩, 입력 중 표시, 인스펙터 카드가 함께 쓴다.
class MemberListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum class Role
    {
        MEMBER_ID = Qt::UserRole + 1,
        NAME,
        COLOR,
        STATUS,
        STATUS_LABEL,
        MODEL_LABEL,
        CLI_PATH,
        CALLS,
        LAST_SECONDS,
        LAST_ERROR,
        IS_ENABLED,
        IS_BOOSTING
    };

    MemberListModel(RoomEngine* engine, AgentHub* hub, QObject* parent = nullptr);

    void Refresh(const QString& memberId);
    void RefreshAll();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QString StatusLabel(const QString& memberId, const QString& status) const;

    RoomEngine* engine;
    AgentHub* hub;
};
