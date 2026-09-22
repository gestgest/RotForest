
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GaugeZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMyPlayer;

UCLASS(Abstract) //부모
class ZOMBIEHUNTER1_API AGaugeZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AGaugeZone();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 언로드 전에 저장해둔 상태를 재생성된 발판에 주입한다. 게이지 표시(Progress)와 BP 이벤트까지 갱신. 
	void RestorePadState(int32 InFilledAmount, int32 InRequiredAmount, bool bInConsumed);

protected:
	// [Component]
	// 밟는 영역(트리거). 이 박스 안에 플레이어가 들어오면 게이지가 찬다. 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GaugeZone")
	UBoxComponent* TriggerBox;

	// 발판 바닥 메시(선택). BP에서 평평한 큐브/플레인 메시를 지정해 시각화. 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GaugeZone")
	UStaticMeshComponent* PadMesh;


	// [Variable]
	// 결제(돈 소비) 간격(초). 발판 위에 서 있는 동안 이 간격마다 MoneyPerPayment씩 빠진다. 작을수록 빨리 참.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.02"), Category = "GaugeZone")
	float Fill_Interval = 0.15f;

	// 한 번 완성되면 더 이상 작동하지 않게 할지(일회성 발판).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone")
	bool bOneShot = false;

	// 완성 후 다시 채울 수 있게 되기까지의 쿨다운(초). bOneShot이 false일 때만 의미 있음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"), Category = "GaugeZone")
	float Cooldown = 5.0f;

	// 현재 게이지(0.0 ~ 1.0). PaidMoney / MaxMoney로 계산되는 표시용 값. 
	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	float Progress = 0.0f;

	// 이 발판에 지금까지 누적해서 낸 돈(원). MaxMoney에 도달하면 완성된다.
	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	int32 FilledAmount = 0;

	// 완성에 드는 총량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "GaugeZone")
	int32 RequiredAmount = 5;

	// 현재 구역 안에 플레이어가 서 있는지.
	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	bool bPlayerInside = false;


	// [Debug]
	// 켜면 발판 위에 기본 디버그 게이지 바를 그려 BP 위젯 없이도 진행도를 확인할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGauge = true;


	// 한 번 채우기를 시도한다. 실패하면 내부 실패함수를 발동
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) PURE_VIRTUAL(AGaugeZone::TryFillOnce, return false;);

	// 게이지가 가득 찼을 때 서브클래스가 할 일(동료 소환/무기 강화 등). Payer는 항상 유효.
	virtual void HandleZoneFilled(AMyPlayer* Payer) {} 

	// 플레이어가 막 올라왔을 때. RequiredAmount를 그때그때 다시 잡을 때 쓴다.
	virtual void OnPlayerEntered(AMyPlayer* Player) {}

	// 추적하던 플레이어가 발판을 떠났을 때.
	virtual void OnPlayerExited(AMyPlayer* Player) {}

	// 게이지가 가득 찼을 때 호출 — 게이지 리셋 → HandleZoneFilled → BP 이벤트 → 쿨다운/소비 처리.
	void CompleteZone(); 

	// bShowDebugGauge가 켜져 있을 때 발판 위에 진행도 바를 그린다.
	void DrawDebugGauge(); 



	// 현재 구역 안에 있는 플레이어(여러 명 겹쳐도 첫 유효 플레이어 1명만 추적).
	UPROPERTY()
	AMyPlayer* CurrentPlayer = nullptr; 

	// 완성 후 남은 쿨다운(초). 0이면 다시 채울 수 있음.
	float CooldownRemaining = 0.0f; 

	// 다음 결제까지 누적 시간(초). PaymentInterval에 도달하면 한 번 결제한다. 
	float Fill_Timer = 0.0f; 

	// bOneShot 발판이 이미 한 번 작동했는지. 
	bool bConsumed = false; 

public:
	FORCEINLINE int32 GetFilledAmount() const { return FilledAmount; }
	FORCEINLINE int32 GetRequiredAmount() const { return RequiredAmount; }
	FORCEINLINE bool IsConsumed() const { return bConsumed; }

	// 완성까지 남은 양
	FORCEINLINE int32 GetRemainingAmount() const { return FMath::Max(0, RequiredAmount - FilledAmount); }
};
