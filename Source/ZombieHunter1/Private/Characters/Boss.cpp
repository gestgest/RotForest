#include "Characters/Boss.h"

void ABoss::OnDeath()
{
    Super::OnDeath();   // AI정지 + 몽타주중단 + 이동차단 + 콜리전해제 + 경험치 + DeadEnemySignal

    SetLifeSpan(5.0f);  // 보스는 풀 반납이 없으니 시체를 직접 치운다
}

