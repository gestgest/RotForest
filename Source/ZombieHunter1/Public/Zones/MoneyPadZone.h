#pragma once

#include "CoreMinimal.h"
#include "Zones/GaugeZone.h"
#include "MoneyPadZone.generated.h"

class AMyPlayer;
class USoundBase;

/**
 * "돈 넣는 발판" 공통 베이스 (밟으면 게이지가 차는 발판).
 *
 * 플레이어가 박스 트리거 안에 서 있으면 일정 간격으로 돈을 소비해 게이지(Progress)가 차오르고,
 * 가득 차면 HandleZoneFilled()가 호출된다 — 뭘 해줄지는 서브클래스가 결정한다:
 *  - ACompanionSpawnZone : 동료 소환
 *  - AWeaponUpgradeZone  : 플레이어 무기 강화
 */

UCLASS(Abstract) //부모
class ZOMBIEHUNTER1_API AMoneyPadZone : public AGaugeZone
{
	GENERATED_BODY()

public:

	// [Variable]
	// 게이지 한 칸(결제 1회)당 소비하는 돈(원). 이만큼 돈이 없으면 게이지가 안 오른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "1"))
	int32 MoneyPerPayment = 1;

	// 돈 부족 사운드
	UPROPERTY(EditAnywhere, Category = "MoneyPadZone")
	USoundBase* InsufficientFundsSound = nullptr;

protected:
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;
};
