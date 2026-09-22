#pragma once

#include "CoreMinimal.h"
#include "Zones/MoneyPadZone.h"
#include "WeaponUpgradeZone.generated.h"

/**
 * 무기 강화 발판 — 돈 발판(AMoneyPadZone) 위에 "완성 시 플레이어 무기 강화"를 얹는다.
 * 강화 자체(레벨/데미지 증가)는 AMyPlayer::UpgradeWeapon()이 담당하고, 여기선 호출만 한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API AWeaponUpgradeZone : public AMoneyPadZone
{
	GENERATED_BODY()

public:
	/** 강화할 때마다 다음 강화 비용(MaxMoney)을 이만큼 올린다. 0이면 비용 고정. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone|Upgrade", meta = (ClampMin = "0"))
	int32 CostGrowth = 5;

protected:
	// 게이지 완성 → 플레이어 무기 강화 + 다음 강화 비용 인상
	virtual void HandleZoneFilled(AMyPlayer* Payer) override;
};
