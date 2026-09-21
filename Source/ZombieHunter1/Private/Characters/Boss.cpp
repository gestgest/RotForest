#include "Characters/Boss.h"
#include "InfiniteMapGenerator.h" // 클리어 기록을 남길 곳 (POIStates)
#include "Items/WeaponPickup.h" // 바닥에 떨굴 전리품
#include "Items/ItemDataAsset.h"
#include "Kismet/GameplayStatics.h" // FinishSpawningActor

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

    // 드롭은 마을 소속과 무관하다 — bHomeSet 체크보다 위에 둬야 소속 없는 보스도 전리품을 남긴다
    SpawnDropPickup();

    // 여기서 기록하지 않으면 청크가 언로드될 때 시체까지 Destroy되고,
    // 다시 방문했을 때 생성기가 아무것도 모른 채 풀피 보스를 새로 세운다.
    if (!bHomeSet)
    {
        return;
    }

    //죽였다는 값 설정
    if (AInfiniteMapGenerator* Generator = HomeGenerator.Get())
    {
        Generator->MarkBossKilled(HomeChunk);
    }
}

void ABoss::SpawnDropPickup()
{
    UWorld* World = GetWorld();
    if (!World || !DropPickupClass || DropTable.Num() == 0)
    {
        return;
    }
    // 떨굴 무기 한 자루를 고른다
    UWeaponDataAsset* Drop = DropTable[FMath::RandRange(0, DropTable.Num() - 1)];
    if (!Drop)
    {
        return;
    }

    const FTransform SpawnTM(FRotator::ZeroRotator, GetActorLocation());

    // 지연 스폰: WeaponPickup의 BeginPlay가 WeaponItemData->Mesh를 읽어 외형을 꽂는다.
    // 그 전에 데이터를 넣어야 무기가 보인다.
    AWeaponPickup* Pickup = World->SpawnActorDeferred<AWeaponPickup>(
        DropPickupClass, SpawnTM, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (!Pickup)
    {
        return;
    }

    Pickup->WeaponItemData = Drop;

    UGameplayStatics::FinishSpawningActor(Pickup, SpawnTM);

    // 청크가 언로드될 때 같이 치워지게 등록 — 안 하면 아무도 안 치우는 미아가 된다
    if (AInfiniteMapGenerator* Generator = HomeGenerator.Get())
    {
        Generator->RegisterChunkActor(HomeChunk, Pickup);
    }
}
