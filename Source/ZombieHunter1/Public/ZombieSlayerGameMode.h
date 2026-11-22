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
class USoundBase;

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
	void SetEnemiesFrozen(bool bFrozen);

	void OnBossKilled();

private:
	void init();

	void initEnemy(int index);
	void initCoin(int index);

	void Spawn();
	void SpawnCoin();
	void UpdateWave();

	void UpdateRot();


	// 너무 멀어졌거나 낙하한 적을 시야 밖 스폰 링으로 재배치 (LeashCheckInterval마다 호출) 
	void RecycleFarEnemies();
	void RecycleFarCoins(const FVector& PlayerLocation);
	void ApplyRotToEnemy(AEnemy* Enemy) const;
	void SpawnBurst(const FVector& PlayerLocation, int32 Level);
	void SpawnDefaultEnemy(const FVector& PlayerLocation);
	void WakeEnemyAt(AEnemy* Enemy, const FVector& Location); // 쉬고 있는 적 호출
	AEnemy* FindDormantEnemy() const;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(AllowPrivateAccess = "True"), Category = "Spawning")
	int Max_Enemy_Size = 30;

	// 레벨1 => StartEnemyCount, 레벨당 +EnemyPerLevel, 최대 Max_Enemy_Size
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 StartEnemyCount = 5;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 EnemyPerLevel = 4;

	// 레벨1 => StartCoinCount, 레벨당 +CoinPerLevel, 최대 Max_Coin_Size
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 StartCoinCount = 5;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 CoinPerLevel = 5;

	// 보스 하나 잡을 때 마다 
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 CoinPerBossKill = 2;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 DefaultSpawnEnemyMin = 1;

	// 스폰 주기
	UPROPERTY(EditAnywhere, Category = "Spawning")
	int32 DefaultSpawnEnemyMax = 3;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawning")
	int Max_Coin_Size = 40;

	// 틱마다 코인 묶음이 나올 확률 => 0.1이라면 0.5초 간격에 평균 5초 
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = "0", ClampMax = "1"))
	float CoinChance = 0.1f;

	// [웨이브]
	// 습격 오는 시간대
	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	USoundBase* BurstWarningSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	float BurstDuration = 20.f;

	// 휴식기
	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	float ClamDuration = 60.f;


	// 평상시 적 오는량 
	// ㄴ 그냥 값을 올려놓고 평상시 오는 적 확률을 매우 낮추자
	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	float EnemyChance = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	float BurstEnemyChance = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Spawning|Wave")
	float BurstWarningTime = 5.f;

	UPROPERTY(VisibleDefaultsOnly, Category = "Spawning|Wave") // 구경 용도
	bool bInBurst = true; // 버스트 인지 아닌지

	//[Rot]
	UPROPERTY(EditAnywhere, Category = "Rot")
	float RotGraceTime = 60.f;

	UPROPERTY(EditAnywhere, Category = "Rot")
	float RotPerMinute = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Rot")
	float BossRotPerKill = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Rot")
	float BossRotRampTime = 60.f;

	UPROPERTY(EditAnywhere, Category = "Rot")
	float HPPerRot = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Rot")
	float DamagePerRot = 0.3f;

	// 보스 전용 => 잡몹의 절반
	UPROPERTY(EditAnywhere, Category = "Rot|Boss")
	float BossHPPerRot = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Rot|Boss")
	float BossDamagePerRot = 0.15f;


	UPROPERTY(EditAnywhere, Category = "Rot|Debug")
	bool bDebugRot = false;




	// UPROPERTY 필수 — 안 붙이면 적/코인이 Destroy될 때 이 raw 포인터가 댕글링이 되어
	// SpawnEnemy 등에서 역참조 시 크래시(액세스 위반)가 난다. UPROPERTY면 파괴 시 자동으로 null이 됨.
	UPROPERTY()
	TArray<AEnemy*> enemyPool;

	UPROPERTY()
	TArray<ACoin*> CoinPool;


	// 무한맵 생성기 (StartPlay에서 레벨에서 찾아 캐시). 스폰 링이 마을(안전지대)을 피하도록 질의한다. 
	UPROPERTY()
	TObjectPtr<AInfiniteMapGenerator> MapGenerator;

	int enemy_size = 0;
	int CoinSize = 0;

	float WavePhaseTime = 0.0f; // 시간
	bool bForceFill = false; // burst 시작되면 바로 초반에 막 나오는 용도
	bool bWarned = false;

	float RotUpdateInterval = 1.0f;
	float RotElapsedTime = 0.0f;
	float RotFromTime = 0.0f;
	float RotFromBossCurrent = 0.0f;

	// 잡은 보스 수 => 부패도 목표치, 코인 상한이 이 값에서 계산됨
	int32 BossKillCount = 0;

	FTimerHandle RotTimerHandle;

	UPROPERTY()
	FTimerHandle SpawnTimerHandle; //타이머 설정 변수


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

	// 이 Z(cm) 아래로 떨어진 적은 어떤 이유로든 즉시 회수 (리쉬가 놓친 케이스의 안전망) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Enemy|Leash")
	float FallZThreshold = -500.0f;

	/** 리쉬 검사 주기(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Enemy|Leash")
	float LeashCheckInterval = 1.0f;

	FTimerHandle LeashTimerHandle;


	//////////////////////////////////////////////////////////////////////////
	// 스폰 링 — 적/코인을 플레이어 눈앞이 아니라 "시야 밖"에 생성해 탐험하다 발견하게 한다.

	// 스폰 최소 거리(cm). 탑다운 카메라 시야보다 커야 눈앞 팝인이 안 보인다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawning")
	float SpawnMinDistance = 2200.0f;

	// 스폰 최대 거리(cm). 리쉬(LeashDistance)와 청크 언로드 경계(~6000)보다 안쪽이어야 한다. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"), Category = "Spawning")
	float SpawnMaxDistance = 3500.0f;

public:
	float GetRot() const { return RotFromTime + RotFromBossCurrent; }

	// 보스 BeginPlay에서 호출
	void ApplyRotToBoss(AEnemy* Boss) const;

private:

};
