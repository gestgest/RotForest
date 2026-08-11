#include "Characters/Boss.h"
#include "InfiniteMapGenerator.h" // 클리어 기록을 남길 곳 (POIStates)

void ABoss::SetHome(AInfiniteMapGenerator* InGenerator, const FIntPoint& InCenterChunk)
{
    HomeGenerator = InGenerator;
    HomeChunk = InCenterChunk;
    bHomeSet = (InGenerator != nullptr);
}

void ABoss::OnDeath()
{
    Super::OnDeath();   // AI정지 + 몽타주중단 + 이동차단 + 콜리전해제 + 경험치 + DeadEnemySignal

    SetLifeSpan(5.0f);  // 보스는 풀 반납이 없으니 시체를 직접 치운다

    // 여기서 기록하지 않으면 청크가 언로드될 때 시체까지 Destroy되고,
    // 다시 방문했을 때 생성기가 아무것도 모른 채 풀피 보스를 새로 세운다.
    if (!bHomeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Boss] Home 미설정 — 클리어가 기록되지 않는다(생성기가 스폰한 보스가 아님?)"));
        return;
    }

    if (AInfiniteMapGenerator* Generator = HomeGenerator.Get())
    {
        Generator->MarkBossKilled(HomeChunk);
    }
}
