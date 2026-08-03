// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieSlayerGameMode.h"
#include "InfiniteMapGenerator.h"


//begin
void AZombieSlayerGameMode::StartPlay()
{
    Super::StartPlay();

    // 무한맵 생성기 캐시 — 스폰 링이 마을(안전지대)을 피하도록 질의할 대상.
    // 없는 레벨(무한맵이 아닌 맵)이면 null로 남고, 마을 회피 없이 기존과 동일하게 동작한다.
    TArray<AActor*> FoundGenerators;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInfiniteMapGenerator::StaticClass(), FoundGenerators);
    if (FoundGenerators.Num() > 0)
    {
        MapGenerator = Cast<AInfiniteMapGenerator>(FoundGenerators[0]);
    }

    AZombieSlayerGameMode::init();

    // 풀 전체를 5초 기다리지 않고 즉시 링에 배치 (드립피드 대신 배치 스폰)
    AZombieSlayerGameMode::spawn();

    // 이후로는 5초마다 spawn()을 불러 죽어서 빈 슬롯만 다시 채운다
    GetWorldTimerManager().SetTimer(
        SpawnTimerHandle,           // 타이머 핸들
        this,                        // 호출할 객체
        &AZombieSlayerGameMode::spawn,  // 호출할 함수
        5.0f,                        // 간격 (5초)
        true                         // 반복 여부 (true = 반복)
    );

    // 리쉬 검사: 뒤처져 지형을 잃는 적을 주기적으로 회수
    GetWorldTimerManager().SetTimer(
        LeashTimerHandle,
        this,
        &AZombieSlayerGameMode::RecycleFarEnemies,
        LeashCheckInterval,
        true
    );
}

void AZombieSlayerGameMode::init()
{
    enemy_size = 0;
    coin_size = 0;


    for (int i = 0; i < MAX_ENEMY_SIZE; i++)
    {
        initEnemy(i);
    }
    for (int i = 0; i < MAX_COIN_SIZE; i++)
    {
        initCoin(i);
    }
}


void AZombieSlayerGameMode::initEnemy(int index)
{
    FVector SpawnLocation = FVector(0, 0, 90);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // SpawnParameters 설정
    FActorSpawnParameters spawnParams;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Enemy 스폰
    AEnemy* newEnemy = GetWorld()->SpawnActor<AEnemy>(
        EnemyClass,
        SpawnLocation,
        SpawnRotation,
        spawnParams
    );

    newEnemy->SetID(index);

    if (newEnemy)
    {
#if WITH_EDITOR
        newEnemy->SetFolderPath(TEXT("Spawned/Enemies")); // 아웃라이너 정리용 (에디터 전용)
#endif
        enemyPool.Add(newEnemy);  // 풀에 추가
        newEnemy->EnterPoolDormancy();  // 숨김 + 콜리전/이동/틱 정지 (숨김만으론 중력에 낙하)
    }
}

void AZombieSlayerGameMode::initCoin(int index)
{
    FVector SpawnLocation = FVector(0, 0, 90);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // SpawnParameters 설정
    FActorSpawnParameters spawnParams;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACoin* newCoin = GetWorld()->SpawnActor<ACoin>(
        CoinClass,
        SpawnLocation,
        SpawnRotation,
        spawnParams
    );

    newCoin->setID(index);
    newCoin->SetCanGet(false);

    if (newCoin)
    {
#if WITH_EDITOR
        newCoin->SetFolderPath(TEXT("Spawned/Coins")); // 아웃라이너 정리용 (에디터 전용)
#endif
        coinPool.Add(newCoin);
        newCoin->SetActorHiddenInGame(true);  // 비활성화
    }
}
void AZombieSlayerGameMode::spawn()
{
    SpawnEnemy();
    spawnCoin();
}

// 풀에서 대기 중(숨김) 상태인 적을 전부 한 번에 깨워 시야 밖 링에 배치한다.
// 게임 시작 직후엔 풀 전체(MAX_ENEMY_SIZE)가 한꺼번에 깨어나고, 이후엔 죽어서 빈 슬롯만 채워진다.
void AZombieSlayerGameMode::SpawnEnemy()
{
    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!myPlayer)
    {
        return;
    }

    const FVector playerLocation = myPlayer->GetActorLocation();
    const FVector upVector(0, 0, 90.0f);

    //전부 깨운다.
    for (AEnemy* enemy : enemyPool)
    {
        // 파괴됐거나 무효한 풀 엔트리, 이미 활성 상태인 적은 건너뛴다.
        if (!IsValid(enemy) || !enemy->IsHidden())
        {
            continue;
        }

        FNavLocation resultLocation;

        // 시야 밖 스폰 링(Min~Max)에서 위치 찾기 — 눈앞 팝인 대신 탐험하다 발견하는 느낌
        if (!FindReachablePointInRing(playerLocation, resultLocation))
        {
            continue;  // 이번엔 실패, 다음 spawn() 주기에 재시도
        }

        if (!enemy->GetController())
        {
            enemy->OnAIController();
        }

        //왜인지 모르지만 정 가운데가 pivot이라 +90을 해야한다.
        enemy->SetActorLocation(resultLocation.Location + upVector); 
        enemy->WakeFromPool();  // 숨김 해제 + 콜리전/이동/틱 복구
        enemy->SetHP(5);        // 죽었던 적이면 부활 전환(OnRevive)까지 발동

        enemy_size++;
    }
}

// 무한맵 리쉬: 플레이어에서 너무 멀어졌거나(청크 언로드로 지형 상실 직전) 이미 낙하한 적을
// 플레이어 근처 NavMesh 위로 재배치한다. 적이 사라지는 대신 진행 방향에서 다시 나타나 압박 유지.
void AZombieSlayerGameMode::RecycleFarEnemies()
{
    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!myPlayer)
    {
        return;
    }

    const FVector playerLocation = myPlayer->GetActorLocation();
    const FVector upVector(0, 0, 90.0f);

    for (AEnemy* enemy : enemyPool)
    {
        // 풀 대기 중이거나 죽어서 사망 연출 중인 적은 건드리지 않는다
        if (!IsValid(enemy) || enemy->IsHidden() || enemy->GetIsDead())
        {
            continue;
        }

        const FVector enemyLocation = enemy->GetActorLocation();
        const bool bTooFar  = FVector::Dist2D(enemyLocation, playerLocation) > LeashDistance;
        const bool bFalling = enemyLocation.Z < FallZThreshold;
        if (!bTooFar && !bFalling)
        {
            continue;
        }

        // 시야 밖 스폰 링으로 재배치 — 눈앞 순간이동이 보이지 않게. 실패하면 다음 주기에 재시도.
        FNavLocation resultLocation;
        if (FindReachablePointInRing(playerLocation, resultLocation))
        {
            enemy->TeleportForLeash(resultLocation.Location + upVector);
        }
    }
}

// 플레이어 기준 SpawnMinDistance~SpawnMaxDistance 링 안에서 NavMesh 위 지점을 찾는다.
// 장애물 위나 NavMesh 빈 곳에 찍힐 수 있어 여러 번 시도한다.
bool AZombieSlayerGameMode::FindReachablePointInRing(const FVector& Center, FNavLocation& OutLocation) const
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return false;
    }

    for (int32 Attempt = 0; Attempt < 10; ++Attempt)
    {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        const float Dist = FMath::FRandRange(SpawnMinDistance, SpawnMaxDistance);
        const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);

        if (NavSys->ProjectPointToNavigation(Candidate, OutLocation, FVector(400.0f, 400.0f, 1000.0f)))
        {
            // 투영 과정에서 링 안쪽(=시야 안)으로 크게 끌려왔으면 무효 — 팝인 방지
            if (FVector::Dist2D(OutLocation.Location, Center) >= SpawnMinDistance * 0.8f)
            {
                // 마을은 안전지대 — 지점이 마을 발자국 안이면 버리고 재시도 (좀비마을은 허용)
                if (MapGenerator && MapGenerator->IsLocationInVillage(OutLocation.Location))
                {
                    continue;
                }
                return true;
            }
        }
    }
    return false;
}


void AZombieSlayerGameMode::spawnCoin()
{
    int i = 0;

    for (; i < coinPool.Num(); i++)
    {
        if (!IsValid(coinPool[i]))
        {
            continue;
        }
        if (coinPool[i]->IsHidden())
        {
            break;
        }
    }

    //Max (또는 쓸 수 있는 코인이 없음)
    if (i >= coinPool.Num())
    {
        return;
    }

    ACoin* coin = coinPool[i];

    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!myPlayer)
    {
        return;
    }

    FNavLocation resultLocation;

    // 코인도 시야 밖 링에 생성 — 돌아다니다 "발견"하는 보상
    bool bSuccess = FindReachablePointInRing(myPlayer->GetActorLocation(), resultLocation);

    FVector upVector(0, 0, 90.0f);

    if (bSuccess)
    {
        coin->SetActorLocation(resultLocation.Location + upVector);
        coin->SetActorEnableCollision(true);
        coin->SetActorHiddenInGame(false);
        coin->SetCanGet(true);
    }
    else
    {
        return;
    }

    coin_size++;
}

//signal을 받아야지 될듯
void AZombieSlayerGameMode::DieEnemy(int index)
{
    enemy_size--;
    if (enemyPool.IsValidIndex(index) && IsValid(enemyPool[index]))
    {
        enemyPool[index]->EnterPoolDormancy();  // 풀로 반납 (숨김 + 콜리전/이동/틱 정지)
    }
}

//signal을 받아야지 될듯
void AZombieSlayerGameMode::DestroyCoin(int index)
{
    coin_size--;
    if (coinPool.IsValidIndex(index) && IsValid(coinPool[index]))
    {
        coinPool[index]->SetCanGet(false);
        coinPool[index]->SetActorHiddenInGame(true);  // 비활성화
    }
}

// 디아블로식 사망 연출 — 플레이어가 죽으면(OnDeath) 적들의 시간을 멈추고,
// 부활하면(OnRevive) 다시 흐르게 한다. 플레이어 자신은 안 얼리므로 죽음 애니메이션은 그대로 재생된다.
void AZombieSlayerGameMode::SetEnemiesFrozen(bool bFrozen)
{
    for (AEnemy* enemy : enemyPool)
    {
        if (IsValid(enemy))
        {
            enemy->SetFrozen(bFrozen);
        }
    }

    // 죽어 있는 동안 새 적이 스폰되거나 리쉬 재배치가 돌면 연출이 깨진다 — 타이머도 같이 멈춘다.
    if (bFrozen)
    {
        GetWorldTimerManager().PauseTimer(SpawnTimerHandle);
        GetWorldTimerManager().PauseTimer(LeashTimerHandle);
    }
    else
    {
        GetWorldTimerManager().UnPauseTimer(SpawnTimerHandle);
        GetWorldTimerManager().UnPauseTimer(LeashTimerHandle);
    }
}
