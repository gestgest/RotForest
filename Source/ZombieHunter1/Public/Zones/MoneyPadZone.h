#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoneyPadZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMyPlayer;

/**
 * "돈 넣는 발판" 공통 베이스 (밟으면 게이지가 차는 발판).
 *
 *
 * 플레이어가 박스 트리거 안에 서 있으면 일정 간격으로 돈을 소비해 게이지(Progress)가 차오르고,
 * 가득 차면 HandleZoneFilled()가 호출된다 — 뭘 해줄지는 서브클래스가 결정한다:
 *  - ACompanionSpawnZone : 동료 소환
 *  - AWeaponUpgradeZone  : 플레이어 무기 강화
 *
 * UI는 OnProgressChanged / OnZoneCompleted (BlueprintImplementableEvent)로 위젯에 연동하거나,
 * bShowDebugGauge로 발판 위에 기본 게이지 바를 띄워 BP 없이도 바로 확인할 수 있다.
 *
 * 프로퍼티/이벤트 이름은 옛 ACompanionSpawnZone 시절 그대로다(카테고리 "SpawnZone" 포함) —
 * 기존 BP_CompanionSpawnZone에 저장된 값과 이벤트 노드가 깨지지 않게 유지한다.
 */
UCLASS(Abstract) //부모
class ZOMBIEHUNTER1_API AMoneyPadZone : public AActor
{
	GENERATED_BODY()

public:
	AMoneyPadZone();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// [Component]
	// 밟는 영역(트리거). 이 박스 안에 플레이어가 들어오면 게이지가 찬다. 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnZone")
	UBoxComponent* TriggerBox; //remove

	// 발판 바닥 메시(선택). BP에서 평평한 큐브/플레인 메시를 지정해 시각화. 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnZone")
	UStaticMeshComponent* PadMesh;  //remove



	// [Variable]
	// 게이지 한 칸(결제 1회)당 소비하는 돈(원). 이만큼 돈이 없으면 게이지가 안 오른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "1"))
	int32 MoneyPerPayment = 1;

	/** 결제(돈 소비) 간격(초). 발판 위에 서 있는 동안 이 간격마다 MoneyPerPayment씩 빠진다. 작을수록 빨리 참. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "0.02"))
	float PaymentInterval = 0.15f; //remove

	/** 한 번 완성되면 더 이상 작동하지 않게 할지(일회성 발판). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone")
	bool bOneShot = false; //remove

	/** 완성 후 다시 채울 수 있게 되기까지의 쿨다운(초). bOneShot이 false일 때만 의미 있음. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "0.0"))
	float Cooldown = 5.0f; //remove

	/** 현재 게이지(0.0 ~ 1.0). PaidMoney / MaxMoney로 계산되는 표시용 값. */
	UPROPERTY(BlueprintReadOnly, Category = "SpawnZone")
	float Progress = 0.0f; //remove

	/** 이 발판에 지금까지 누적해서 낸 돈(원). MaxMoney에 도달하면 완성된다. */
	UPROPERTY(BlueprintReadOnly, Category = "SpawnZone")
	int32 PaidMoney = 0; //remove

	/** 완성에 드는 총 돈(원). 게이지가 가득 차려면 이만큼 누적 결제해야 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "1"))
	int32 MaxMoney = 5; //remove

	/** 현재 구역 안에 플레이어가 서 있는지. */
	UPROPERTY(BlueprintReadOnly, Category = "SpawnZone")
	bool bPlayerInside = false; //remove

	///////////////////////////////////////////////////////////////////////////////////
	// Variable - debug
	///////////////////////////////////////////////////////////////////////////////////

	// 켜면 발판 위에 기본 디버그 게이지 바를 그려 BP 위젯 없이도 진행도를 확인할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone|Debug")
	bool bShowDebugGauge = true; //remove


	// [블루프린트 이벤트] (UI 연동용)
	// 게이지가 바뀔 때마다 호출  (NewProgress: 0~1).
	UFUNCTION(BlueprintImplementableEvent, Category = "SpawnZone")
	void OnProgressChanged(float NewProgress); //remove

	// 게이지가 가득 차서 보상(소환/강화)을 준 직후 호출. 이펙트/사운드 연출에 사용.
	// 파라미터 이름 Recruiter는 옛 이름 그대로 — 바꾸면 기존 BP 이벤트 노드의 핀이 깨진다.
	UFUNCTION(BlueprintImplementableEvent, Category = "SpawnZone")
	void OnZoneCompleted(AMyPlayer* Recruiter); //remove

	// 발판 위에 있지만 돈이 부족해 결제에 실패한 순간 호출. "돈 부족" UI/사운드에 사용.
	UFUNCTION(BlueprintImplementableEvent, Category = "SpawnZone")
	void OnInsufficientFunds();

	///////////////////////////////////////////////////////////////////////////////////
	// 상태 영속 (무한맵 청크 언로드/재생성 시 게이지 보존용)
	///////////////////////////////////////////////////////////////////////////////////

	/** 일회성 발판이 이미 쓰였는지 — 언로드 시 저장용 읽기 (bConsumed는 protected라 밖에서 못 읽음) */
	bool IsConsumed() const { return bConsumed; }  //remove

	// 언로드 전에 저장해둔 상태를 재생성된 발판에 주입한다. 게이지 표시(Progress)와 BP 이벤트까지 갱신. 
	void RestorePadState(int32 InPaidMoney, int32 InMaxMoney, bool bInConsumed); //remove



protected:
	// 게이지가 가득 찼을 때 서브클래스가 할 일(동료 소환/무기 강화 등). Payer는 항상 유효.
	virtual void HandleZoneFilled(AMyPlayer* Payer) {} //remove

	// 게이지가 가득 찼을 때 호출 — 게이지 리셋 → HandleZoneFilled → BP 이벤트 → 쿨다운/소비 처리.
	void CompleteZone(); //remove

	// bShowDebugGauge가 켜져 있을 때 발판 위에 진행도 바를 그린다.
	void DrawDebugGauge(); //remove

	// 트리거 오버랩 콜백
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep); //remove

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex); //remove

	// 현재 구역 안에 있는 플레이어(여러 명 겹쳐도 첫 유효 플레이어 1명만 추적).
	UPROPERTY()
	AMyPlayer* CurrentPlayer = nullptr; //remove

	// 완성 후 남은 쿨다운(초). 0이면 다시 채울 수 있음.
	float CooldownRemaining = 0.0f; //remove

	// 다음 결제까지 누적 시간(초). PaymentInterval에 도달하면 한 번 결제한다. 
	float PaymentTimer = 0.0f; //remove

	// bOneShot 발판이 이미 한 번 작동했는지. 
	bool bConsumed = false; //remove
};
