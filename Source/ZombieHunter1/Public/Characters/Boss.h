#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemy.h"

enum class EBossPattern : uint8
{
	Chase, // 평범한 추격
	Windup, // 준비 동작
	Charge, // 돌진
	Recover // 돌진 이후 멈추기
};

#include "Boss.generated.h"

class AInfiniteMapGenerator;
class AWeaponPickup;
class UWeaponDataAsset;
class UMyCanvas;

UCLASS()
class ZOMBIEHUNTER1_API ABoss : public AEnemy
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void OnDeath() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	/** 소속 좀비마을을 알려준다 — 스폰 직후 생성기가 자기 자신과 중심 청크 좌표를 넣어준다.
	 *  이걸 알아야 죽을 때 "어느 마을을 클리어했는지"를 기록할 수 있다. */
	void SetHome(AInfiniteMapGenerator* InGenerator, const FIntPoint& InCenterChunk);

protected:
	virtual bool CanTrack() const override;


	// [변수 드롭]
	// 이 보스가 떨굴 수 있는 무기들.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TArray<UWeaponDataAsset*> DropTable;

	// 바닥에 떨굴 픽업 액터 (BP_WeaponPickup)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TSubclassOf<AWeaponPickup> DropPickupClass;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stat")
	FText BossName = FText::FromString(TEXT("Boss"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stat")
	float CombatRadius = 1500.0f; //15m

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stat")
	float LeaveCombatRadius = 3000.0f;


	// [변수 : 패턴]
	// 돌진 주사위 쿨타임
	UPROPERTY(EditDefaultsOnly, Category="Pattern|Charge")
	float ChargeCheckInterval = 1.2f; // 이런거 주사위 쿨타임도 1초가 아닌 아예 랜덤느낌으로 하고 싶다.

	// 돌진 확률
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeChance = 0.3f;

	// 돌진 최소 길이
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeMinDistance = 300.0f; // 거리가 가깝다면

	// 돌진 최대 길이
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeMaxDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float WindupTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeSpeed = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float RecoverTime = 1.0f;

	// 돌진 데미지 배수
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeDamageMultiplier = 2.0f;

	// 충돌 범위
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeHitRadius = 120.0f;
	// 충돌 범위
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeDamagedHeight = 600.0f;

	// 충돌 범위
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargePower = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	UAnimMontage* WindupMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	UAnimMontage* ChargeMontage = nullptr;

	// 공격 적중 시 재생할 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	USoundBase* ChargeSound = nullptr;

private:
	// [Tick 함수]
	void UpdatePattern(float DeltaTime);

	// 패턴 상태
	EBossPattern Pattern = EBossPattern::Chase;
	FVector ChargeDir = FVector::ZeroVector;
	TWeakObjectPtr<ACombatCharacter> PatternTarget;

	// 상태를 기록할 생성기. 약참조 — 레벨 종료 중이면 이미 사라졌을 수 있다. 
	TWeakObjectPtr<AInfiniteMapGenerator> HomeGenerator;
	FIntPoint HomeChunk = FIntPoint::ZeroValue;
	

	// (0,0)도 유효한 청크 좌표라 좌표값만으로는 "설정됨"을 구분할 수 없다 — 별도 플래그가 필요.
	bool bHomeSet = false;
	bool bInCombat = false;  // 전투중. 보스바 관련 기능을 조율
	bool bChargeHit = false; // 한번이라도 돌진할때 맞은 경우
	 
	// 패턴
	float PatternTime = 0.0f;
	float ChargeRemainTime = 0.0f; // 돌진 주사위 남은 시간
	float TempWalkSpeed = 0.0f;  //백업 걷기 스피드


	// 잡다한 함수들
	void SpawnDropPickup();


	void UpdateEncounter();
	UMyCanvas* GetPlayerCanvas() const;

	void SetPattern(EBossPattern NewPattern);
	void TickCharge();
	void TryStartCharge();
};
