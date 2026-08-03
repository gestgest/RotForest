#pragma once
#include <vector>

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Characters/Enemy.h"
#include "Coin.h"
#include "Characters/MyPlayer.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"  // ATargetPoint 클래스 정의

#include "ZombieSlayerGameMode.generated.h"

class AInfiniteMapGenerator;

UCLASS()
class ZOMBIEHUNTER1_API AZombieSlayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;


	// 플레이어 주변 링(Min~Max 거리) 안의 NavMesh 위 랜덤 지점을 찾는다. 실패 시 false. 
	bool FindReachablePointInRing(const FVector& Center, FNavLocation& OutLocation) const;

	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

	UFUNCTION(BlueprintCallable)
	void DieEnemy(int index);

	UFUNCTION(BlueprintCallable)
	void DestroyCoin(int index);




	// 사망 연출 — 플레이어가 죽으면 적들의 시간을 멈춘다.
	// 풀의 모든 적 시간 정지/해제 + 스폰·리쉬 타이머 일시정지/재개.
	// 플레이어 AMyPlayer::OnDeath(정지) / OnRevive(해제)가 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Death")
	void SetEnemiesFrozen(bool bFrozen);

private:
	void init();

	void initEnemy(int index);
	void initCoin(int index);

	void spawn();
	void spawnCoin();




	// 너무 멀어졌거나 낙하한 적을 시야 밖 스폰 링으로 재배치 (LeashCheckInterval마다 호출) 
	void RecycleFarEnemies();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(AllowPrivateAccess = "True"), Category = "Pool")
	int Max_Enemy_Size = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Pool")
	int Max_Coin_Size = 10;



	// UPROPERTY 필수 — 안 붙이면 적/코인이 Destroy될 때 이 raw 포인터가 댕글링이 되어
	// SpawnEnemy 등에서 역참조 시 크래시(액세스 위반)가 난다. UPROPERTY면 파괴 시 자동으로 null이 됨.
	UPROPERTY()
	TArray<AEnemy*> enemyPool;
	UPROPERTY()
	TArray<ACoin*> coinPool;

	/** 무한맵 생성기 (StartPlay에서 레벨에서 찾아 캐시). 스폰 링이 마을(안전지대)을 피하도록 질의한다. */
	UPROPERTY()
	TObjectPtr<AInfiniteMapGenerator> MapGenerator;

	int enemy_size = 0;
	int coin_size = 0;




	UPROPERTY()
	FTimerHandle SpawnTimerHandle; //타이머 설정 변수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawners")
	TArray<ATargetPoint*> Spawners;


	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<AEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Coin")
	TSubclassOf<AActor> CoinClass;

	//////////////////////////////////////////////////////////////////////////
	// 리쉬(leash) — 무한맵에서 뒤처져 청크 언로드로 지형을 잃는 적을 회수해 재배치한다.

	/** 플레이어에서 이 거리(cm) 이상 멀어진 적은 플레이어 근처로 재배치.
	 *  청크 언로드 경계(ViewRadiusInChunks × ChunkSize ≈ 6000)보다 안쪽이어야 낙하 전에 회수된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Enemy|Leash")
	float LeashDistance = 4500.0f;

	/** 이 Z(cm) 아래로 떨어진 적은 어떤 이유로든 즉시 회수 (리쉬가 놓친 케이스의 안전망) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Enemy|Leash")
	float FallZThreshold = -500.0f;

	/** 리쉬 검사 주기(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Enemy|Leash")
	float LeashCheckInterval = 1.0f;

	FTimerHandle LeashTimerHandle;


	//////////////////////////////////////////////////////////////////////////
	// 스폰 링 — 적/코인을 플레이어 눈앞이 아니라 "시야 밖"에 생성해 탐험하다 발견하게 한다.

	/** 스폰 최소 거리(cm). 탑다운 카메라 시야보다 커야 눈앞 팝인이 안 보인다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawning")
	float SpawnMinDistance = 2200.0f;

	/** 스폰 최대 거리(cm). 리쉬(LeashDistance)와 청크 언로드 경계(~6000)보다 안쪽이어야 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawning")
	float SpawnMaxDistance = 3500.0f;
};
