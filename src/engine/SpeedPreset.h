#pragma once

#include <QString>

// 대화 속도 프리셋 (초 단위).
//   read     새 메시지를 보고 반응하기까지 [최소, 최대]
//   idle     침묵 깨기를 끈 경우, 각자 혼잣말 턴을 받기까지
//   spark    방이 이만큼 조용하면 한 명을 골라 침묵을 깨게 한다
//   typing   말풍선 사이 "입력 중" 시간 배율
//   cooldown 한 멤버가 턴을 마친 뒤 다음 턴까지 최소 간격
//   perMin   1분에 AI 말풍선 최대 개수 (호출받은 턴은 예외)
struct SpeedPreset {
    double readLo, readHi;
    double idleLo, idleHi;
    double sparkLo, sparkHi;
    double typing;
    int cooldown;
    int perMin;
};

inline SpeedPreset speedPreset(const QString& name)
{
    if (name == QLatin1String("slow"))
        return { 8, 22, 100, 220, 300, 600, 1.3, 15, 6 };
    if (name == QLatin1String("fast"))
        return { 1, 6, 25, 60, 70, 160, 0.6, 3, 18 };
    return { 4, 12, 50, 120, 150, 330, 1.0, 8, 10 };
}
