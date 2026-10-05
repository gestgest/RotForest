#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemy.h"


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


private:
	// 상태를 기록할 생성기. 약참조 — 레벨 종료 중이면 이미 사라졌을 수 있다. 
	TWeakObjectPtr<AInfiniteMapGenerator> HomeGenerator;

	FIntPoint HomeChunk = FIntPoint::ZeroValue;

	// (0,0)도 유효한 청크 좌표라 좌표값만으로는 "설정됨"을 구분할 수 없다 — 별도 플래그가 필요.
	bool bHomeSet = false;

	// 전투중. 보스바 관련 기능을 조율
	bool bInCombat = false; 

	void SpawnDropPickup();

	void UpdateEncounter();
	UMyCanvas* GetPlayerCanvas() const;

};
