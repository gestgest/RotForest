// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CombatCharacter.h"
#include "UI/MyCanvas.h"
#include "MyPlayer.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class UNavigationInvokerComponent;
class UVirtualJoystick;
class UTexture2D;
class UJobComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class ACompanion;

UCLASS()
class ZOMBIEHUNTER1_API AMyPlayer : public ACombatCharacter
{
	GENERATED_BODY()

public:
	AMyPlayer();

protected:
	virtual void BeginPlay() override;

	// 이동/입력을 끄고 사망 UI를 켠다.
	virtual void OnDeath() override; 

	// 죽음 → 부활(ReStart의 SetHP) 
	virtual void OnRevive() override;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//~ Begin Death
	// 파라미터 이름은 bDead — 베이스(ACombatCharacter)의 IsDead 멤버와 겹치면 UHT가 shadowing 에러를 냄.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player")
	void CheckDeath(bool bDead);
	//~ End Death


	/*
		여담으로 get set같은 프로퍼티와 부품 함수라면 밑 public에 넣어라.
		1. 핵심 메인 함수
		2. 핵심 메인 변수
		3. 부품 변수
		4. 부품 함수 => 프로퍼티

		private도 변수는 앞에
	*/


	// 강화 발판 AWeaponUpgradeZone이 호출 => WeaponLevel 증가 + 현재 직업의 Damage 상승.
	UFUNCTION(BlueprintCallable, Category = "Player|Weapon")
	void UpgradeWeapon();


	// 스폰존의 생성 함수 HandleZoneFilled에서 호출
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void RecruitCompanion(TSubclassOf<UJobComponent> JobComponent);



	//근데 안 쓰이는 거 같다?
	// 키보드(WASD) Enhanced Input(IA_Move)에서 호출. 카메라가 고정된 월드축 기준으로 이동
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void MoveTopDown(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(왼쪽: 이동)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetMoveInput(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(오른쪽: 조준+공격)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetAimInput(FVector2D Value);



	// 하체 yaw 오프셋(도). AnimInstance(UCombatAnimInstance)가 매 프레임 읽는다.
	FORCEINLINE float GetLegYawOffset() const { return LegYawOffset; }

	// 현재 동료 목록(읽기 전용) — 적 타게팅(AEnemy::TrackingPlayer) 등 외부 조회용. 
	FORCEINLINE const TArray<ACompanion*>& GetCompanions() const { return Companions; }

	// Returns TopDownBoom subobject 
	FORCEINLINE USpringArmComponent* GetTopDownBoom() const { return TopDownBoom; }
	// Returns TopDownCamera subobject
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }




protected:
	// [아직 구현 안함] BP에서 이펙트/사운드/무기 외형 교체 등을 구현. => 
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Weapon")
	void OnWeaponUpgraded(int32 NewWeaponLevel);



private: //평범한 변수 및 함수
	////////////////////////////////////////////////////////////////////////
	// ~ Begin Stats
	// HP / Damage 는 베이스(ACombatCharacter)로 이동.
	// 블루프린트에서 읽고 쓸 수 있는 Money 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Money;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Exp = 0;

	// 1레벨 → 2레벨에 필요한 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 ExpBase = 10;

	// 레벨당 필요 경험치 증가량 (필요량 = ExpBase + (Level-1) × ExpGrowth)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 ExpGrowth = 5;
	// ~ End Stats
	////////////////////////////////////////////////////////////////////////





	////////////////////////////////////////////////////////////////////////ㄱ
	// Weapon
	// 강화 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 WeaponLevel = 0;

	// 데미지 증가량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 WeaponDamagePerLevel = 1;
	////////////////////////////////////////////////////////////////////////


	// 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	USoundBase* AttackSound; //MS



	////////////////////////////////////////////////////////////////////////
	//Input Variable
	// 입력 누적값 (게임패드 / 터치를 분리 저장 후 Tick에서 합성)
	FVector2D GamepadMove = FVector2D::ZeroVector;
	FVector2D GamepadAim = FVector2D::ZeroVector;
	FVector2D TouchMove = FVector2D::ZeroVector;
	FVector2D TouchAim = FVector2D::ZeroVector;

	//조이스틱 인스턴스
	UPROPERTY()
	UVirtualJoystick* MoveJoystick;

	UPROPERTY()
	UVirtualJoystick* AimJoystick;

	// 스틱 입력 데드존 => 안 움직임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Input", meta = (AllowPrivateAccess = "true"))
	float InputDeadzone = 0.25f;

	// 마우스 이동: 커서가 캐릭터로부터 이 거리(cm) 안이면 이동 정지 — 가까울 때 방향이 뒤집혀 제자리 진동하는 것 방지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Input", meta = (AllowPrivateAccess = "true"))
	float CursorStopRadius = 60.0f;

	// 캐릭터가 조준/이동 방향으로 회전하는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Movement", meta = (AllowPrivateAccess = "true"))
	float TurnInterpSpeed = 12.0f;
	
	// 켜면 화면에 이동 입력 크기 / 실제 속도 / MaxWalkSpeed를 출력한다. 이동 속도 튜닝용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowSpeedDebug = false;
	////////////////////////////////////////////////////////////////////////




	////////////////////////////////////////////////////////////////////////
	//~ Begin Camera Variable
	// 비스듬한 탑다운 카메라 암 (BP의 기존 CameraBoom과 이름 충돌 방지)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* TopDownBoom;

	// 탑다운 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCamera;

	// 카메라 내려보는 각도(피치). 음수일수록 위에서 내려봄
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraPitch = -35.0f;

	// 카메라 거리(암 길이) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraDistance = 900.0f;
	////////////////////////////////////////////////////////////////////////




	// NavMesh를 플레이어 주변에만 동적으로 생성시키는 인보커 (무한 맵용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Navigation", meta = (AllowPrivateAccess = "true"))
	UNavigationInvokerComponent* NavInvoker;





	////////////////////////////////////////////////////////////////////////┐
	// ~ Begin Animation
	// 
	// 자동 공격 간격(AttackInterval)·직업(DefaultJobClass/CurrentJob)·공격 몽타주(AttackMontage)는
	// 모두 베이스(ACombatCharacter)로 이동했다. 동료(ACompanion)와 똑같은 배선이라 한곳에 모음.
	// 
	// 다리를 이동 방향으로 돌리기 위한 yaw 오프셋(도). 액터 정면(=조준) 기준 이동 방향과의 각도. AnimBP가 읽는다.
	UPROPERTY(BlueprintReadOnly, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawOffset = 0.0f;

	// LegYawOffset 보간 속도(클수록 다리가 이동 방향으로 빨리 돌아감) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawInterpSpeed = 10.0f;

	// LegYawOffset 최대 각도(도). 전방 애니 1개라 이 이상은 허리가 부러져 보여서 막는다(보통 90). 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawMaxAngle = 90.0f;
	// ~ End Animation
	////////////////////////////////////////////////////////////////////////┘



	UPROPERTY()
	APlayerController* PlayerControllerRef = nullptr;

	UPROPERTY()
	AActor* playerStart = nullptr;

	// 공격 타이머(TimeSinceLastAttack)는 베이스(ACombatCharacter::TickAttack)가 관리한다.
	bool bLeftMouseHeld = false;
	bool bRightMouseHeld = false;

	// 직전 유효 커서 방향(월드, 수평). 커서 변환이 실패한 프레임에 이걸 재사용해 끊김 방지.
	FVector LastCursorDir = FVector::ForwardVector;



	////////////////////////////////////////////////////////////////////////┐
	// UI Variable
	UPROPERTY()
	UMyCanvas* CanvasWidget = nullptr;
	////////////////////////////////////////////////////////////////////////┘



	////////////////////////////////////////////////////////////////////////
	// Companion
	/** 스폰할 동료 클래스(BP_Companion 지정). 비우면 섭외 안 됨. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACompanion> CompanionClass;

	/** 최대 동료 수. 이 인원에 도달하면 더 섭외하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion", meta = (AllowPrivateAccess = "true"))
	int32 MaxCompanions = 3;

	/** 동료를 플레이어 기준 어디에 스폰할지 오프셋(cm). 살짝 옆/위로 띄워 바닥 끼임 방지. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion", meta = (AllowPrivateAccess = "true"))
	FVector CompanionSpawnOffset = FVector(-120.0f, 120.0f, 0.0f);

	/** 현재 섭외해 둔 동료들(런타임). 죽으면 정리된다. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion", meta = (AllowPrivateAccess = "true"))
	TArray<ACompanion*> Companions;

	bool CheckCompanion(UWorld* World);
	FTransform SetSpawnTransformCompanion(UWorld* World);
	////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////
	// Debug Function
	// true면 C 키로 디버그용 돈 획득(AddMoney)을 테스트할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugAddMoneyKey = true;
	////////////////////////////////////////////////////////////////////////







	//~ Begin Input
	// 마우스(로스트아크식) 입력을 스틱 포맷으로 변환해 채워서 내보낸다(출력 파라미터).
	void MouseInput(FVector2D& MouseMove, FVector2D& MouseAim);
	void UpdateMovement(float DeltaTime, const FVector2D& Move);
	void UpdateAimAndAttack(float DeltaTime, const FVector2D& Aim, const FVector2D& Move);

	// 매 프레임 속도 방향과 액터 회전으로 LegYawOffset을 갱신한다(Tick에서 호출).
	void UpdateLegYawOffset(float DeltaTime);

	// 마우스(로스트아크식): 우클릭 누르는 동안 커서로 이동, 좌클릭 누르는 동안 커서 방향 공격
	void OnLeftMousePressed();
	void OnLeftMouseReleased();
	void OnRightMousePressed();
	void OnRightMouseReleased();

	// 마우스 커서가 가리키는 지면(플레이어 높이의 수평면) 위치. 카메라 광선과 평면의 교점.
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	// 조이스틱 델리게이트 콜백 (→ SetMoveInput / SetAimInput 로 연결)
	UFUNCTION()
	void OnMoveJoystickMoved(FVector2D Value);

	UFUNCTION()
	void OnAimJoystickMoved(FVector2D Value);
	//~ End Input



	// 초기 셋업 / 내부 헬퍼 (BeginPlay 등 내부에서만 호출)
	void OnTopDownMode();
	void SetJob();
	void SetMoney(int Money);


	// AddExp와 SetCanvasWidget(초기 표시)에서 호출. => 경험치 HUD(텍스트/바) 갱신.
	void UpdateExpUI();



	//아마 부활할때 넣을듯 => ReVived랑 비교해
	void ReStart();


	// 게임패드 아날로그 축 콜백
	void OnMoveX(float Value);
	void OnMoveY(float Value);
	void OnAimX(float Value);
	void OnAimY(float Value);




	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCanvasWidget(UMyCanvas* CW);

public: //PROPERTY FUNCTION

	// HUD 체력바 갱신.
	virtual void SetHP(int32 new_hp) override; //  죽음/부활 "전환" 처리는 베이스가 OnDeath/OnRevive로 호출해준다.


	//차감 성공하면 true
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	bool TrySpendMoney(int32 Amount);


	// 레벨업 처리와 HUD 갱신까지 담당.
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	void AddExp(int32 Amount);


	// 죽었는지
	UFUNCTION(BlueprintCallable)
	bool GetIsDead();








	// ~ Begin Growth Function
	// 현재 레벨에서 다음 레벨까지 필요한 경험치 총량
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	int32 GetExpToNextLevel() const;

	//BP에서 이펙트/사운드/스탯 상승 등을 구현할 예정.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Stats")
	void OnLevelUp(int32 NewLevel);
	// ~ End Growth Function


	//AddCoin
	UFUNCTION(BlueprintCallable)
	void AddMoney();
};
