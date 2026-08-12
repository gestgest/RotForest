# RotForest
언리얼 엔진 5로 제작된 3인칭 서바이벌 아케이드 게임입니다. 플레이어는 적이 배회하는 위험한 공간에서 살아남으며 최대한 많은 자금을 수집해야 합니다. 적들의 추격을 피하면서 전략적으로 움직여 돈을 획득하는 긴장감 넘치는 게임플레이를 제공합니다.

[유튜브 링크](https://youtu.be/MZv_oC9dAoc?si=nLq2Tmy80-yx6WsR)

---
## 프로젝트 설명
### 기능
- 전투 시스템
- 코인 시스템
- 코인, 적 pooling 시스템

### 기간
2025년 12월 15일 ~ 2026년 2월 14일 (2개월)
2026년 6월 15일 ~ 8월 12일 ing (2개월)

### 인원
1인 개발

---
# 에셋 목록

- Content/StarterContent
- Content/ElfArden
- Content/SKnight_modular



---
# 인 게임 플레이

### 전투 시스템
![bandicam 2025-12-14 21-58-13-948](https://github.com/user-attachments/assets/6d7ee84f-4018-4288-8023-b7dcdfd99186)
- 공격에 피격될 때 마다 HP가 1씩 줄어듭니다. 체력 게이지를 통해 실시간으로 생존 상태를 확인할 수 있습니다.
- 좀비들은 플레이어와 마주치면 따라갑니다.

```
bool AEnemy::hit()
{
	bool isAttack = false;

	TArray<FHitResult> hitResults;
	FVector start = GetActorLocation();
	FVector end = start + (GetActorForwardVector() * attackRange);
	

	FCollisionShape sphere = FCollisionShape::MakeSphere(attackRange);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, //TraceChannel
		sphere,
		queryParams
	);

	// Sweep 경로를 선으로 연결
	DrawDebugLine(
		GetWorld(),
		start,
		end,
		FColor::Red,
		false,
		2.0f,
		0,
		2.0f
	);

	for (const FHitResult& hit : hitResults)
	{
		AMyPlayer* hitCharacter = Cast<AMyPlayer>(hit.GetActor());
		if (hitCharacter && hitCharacter->IsPlayerControlled())
		{
			//UE_LOG(LogTemp, Log, TEXT("Hit Player!"));
			hitCharacter->AddHP(-Damage);
			FVector force = GetActorForwardVector() * 500 + FVector(0, 0, 100);
			hitCharacter->LaunchCharacter(force, false, false);
			isAttack = true;
		}
	}
	return isAttack;
}
```

### 코인 수집
![bandicam 2025-12-14 21-58-13-948 (1)](https://github.com/user-attachments/assets/0637a540-b307-41fb-8c95-13b5a1e2f602)
<img width="1320" height="777" alt="image" src="https://github.com/user-attachments/assets/dde600bb-6d93-46c2-bddb-ce46079f8308" />

- 맵에 배치된 코인을 수집하여 점수를 획득합니다.

### GameOver
![bandicam 2025-12-14 21-58-13-948 (2)](https://github.com/user-attachments/assets/d29fe1a3-ba16-4fca-80b8-d2122d538f8b)
- HP가 0이 되면 플레이어는 쓰러집니다.

<img width="773" height="365" alt="image" src="https://github.com/user-attachments/assets/37c655aa-f767-4b2a-8e35-d2fc87c951bc" /><br>
- 이후 버튼을 누르면 HP가 가득 채워진 채 부활합니다.


### 적, 코인 생성
 ![bandicam 2026-02-13 00-58-35-815_success](https://github.com/user-attachments/assets/f7958179-a355-402e-aa4b-c12fcf40be96)<br>
오브젝트 pooling 알고리즘을 활용해서 적과 코인을 주변에 생성합니다.
```

//초기 메모리 생성
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
        enemyPool.Add(newEnemy);  // 풀에 추가

        newEnemy->SetActorHiddenInGame(true);  // 비활성화
    }
}

//적 생성 코드
void AZombieSlayerGameMode::SpawnEnemy()
{
    int i = 0;

    for (; i < enemyPool.Num(); i++)
    {
        if (enemyPool[i]->IsHidden())
        {
            break;
        }
    }
    //Max
    if (i == enemyPool.Num())
    {
        return;
    }

    AEnemy *enemy = enemyPool[i];

    if (!enemy->GetController())
    {
        enemy->SetAIController();
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 
    float radius = 500;


    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    FVector playerLocation = myPlayer->GetActorLocation();
    FNavLocation resultLocation;

    // 플레이어 주변 Radius 내 랜덤 도달가능 위치 찾기
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(playerLocation, radius, resultLocation);

    FVector upVector(0, 0, 90.0f);

    if (bSuccess)
    {
        enemy->SetActorLocation(resultLocation.Location + upVector);
        enemy->SetActorHiddenInGame(false);
        enemy->SetHP(5);
    }
    else
    {
        return;
    }

    enemy_size++;
}
```

### 공격
![bandicam 2026-01-09 02-46-22-305](https://github.com/user-attachments/assets/404c3035-8a47-4845-8f66-2f201bcc42a2)


