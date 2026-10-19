#pragma once

#include "CoreMinimal.h"
#include "Zones/MoneyPadZone.h"
#include "Jobs/JobComponent.h"
#include "CompanionSpawnZone.generated.h"


class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 동료 소환 발판 — 돈 발판(AMoneyPadZone) 공통 로직 위에 "완성 시 동료 소환"만 얹는다.
 * 트리거/결제/게이지/쿨다운/디버그 바는 전부 베이스가 처리한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API ACompanionSpawnZone : public AMoneyPadZone
{
	GENERATED_BODY()


protected:
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;

	// 게이지 완성 → 동료 소환
	virtual void HandleZoneFilled(AMyPlayer* Player) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* IconMeshComp = nullptr;

	//todo 블루프린트에서 호출해줘야함
	UFUNCTION(BlueprintCallable, Category = "Mesh")
	void SetIconMeshComponent(USkeletalMeshComponent* Comp);


	//todo 블루프린트 편집 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Job")
	TMap<EJobType, TSubclassOf<UJobComponent>> JobComponents;

	UFUNCTION(BlueprintCallable, Category = "Mesh")
	void ChangeIconMesh();
	
	//JobType을 보고 JopComponent를 반환
	TSubclassOf<UJobComponent> GetJobComponent();

	virtual FText GetZoneTitle() const override;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Job")
	EJobType JobType;

	// JobType을 바꾸고 아이콘과 게이지 이름을 갱신한다
	void SetJobType(EJobType NewJobType);
};
