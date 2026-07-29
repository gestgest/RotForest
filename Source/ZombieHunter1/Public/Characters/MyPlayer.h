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


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Unreal Event
	////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
	// Sets default values for this character's properties
	AMyPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 살아있음 → 죽음 전환 시 베이스(SetDead)가 1회 호출. 이동/입력을 끄고 사망 UI를 켠다.
	virtual void OnDeath() override;

	// 죽음 → 부활(ReStart의 SetHP) 전환 시 베이스가 1회 호출. 죽을 때 껐던 것을 되돌린다.
	virtual void OnRevive() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Function
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 파라미터 이름은 bDead — 베이스(ACombatCharacter)의 IsDead 멤버와 겹치면 UHT가 shadowing 에러를 냄.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player")
	void CheckDeath(bool bDead);

	// 공격 Notify → 현재 직업의 OnAttackNotify 전달은 베이스(ACombatCharacter)의 기본 구현이 처리한다.

	UFUNCTION(BlueprintCallable)
	void SetCanvasWidget(UMyCanvas* CW);

	//AddCoin
	UFUNCTION(BlueprintCallable)
	void AddMoney();

	// AddHP는 베이스 구현을 그대로 쓴다(내부에서 가상 SetHP를 호출 → 아래 override로 들어옴).

	/** HP 설정 + HUD 체력바 갱신. 죽음/부활 "전환" 처리는 베이스가 OnDeath/OnRevive로 호출해준다. */
	virtual void SetHP(int32 new_hp) override;

	/** 죽었는지 조회만 한다(부작용 없음). 실제 죽음/부활 처리는 OnDeath/OnRevive에서. */
	UFUNCTION(BlueprintCallable)
	bool checkDead();

	void ReStart();

	/** 돈을 Amount만큼 소비 시도. 충분하면 차감하고 true, 부족하면 아무것도 안 하고 false. */
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	bool TrySpendMoney(int32 Amount);

	/** 경험치 획득(적 처치 시 AEnemy::OnDeath가 호출). 레벨업 처리와 HUD 갱신까지 담당. */
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	void AddExp(int32 Amount);

	/** 현재 레벨에서 다음 레벨까지 필요한 경험치 총량 (= ExpBase + (Level-1) × ExpGrowth) */
	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	int32 GetExpToNextLevel() const;

	/** 레벨업 순간 1회 호출 — BP에서 이펙트/사운드/스탯 상승 등을 구현. 여러 레벨을 한 번에 오르면 레벨마다 호출. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Stats")
	void OnLevelUp(int32 NewLevel);

	/** 무기 강화(강화 발판 AWeaponUpgradeZone이 호출) — WeaponLevel 증가 + 현재 직업의 Damage 상승. */
	UFUNCTION(BlueprintCallable, Category = "Player|Weapon")
	void UpgradeWeapon();

	/** 무기 강화 직후 1회 호출 — BP에서 이펙트/사운드/무기 외형 교체 등을 구현. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|Weapon")
	void OnWeaponUpgraded(int32 NewWeaponLevel);

	/** 무기 컴포넌트의 스켈레탈 메시를 교체한다. NewMesh가 null이면 무기를 숨긴다. 직업이 호출. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponMesh(USkeletalMesh* NewMesh);

	/** 동료를 플레이어 근처에 스폰해 따라다니게 한다. 발동 조건은 BP/디버그 키(C)에서 이 함수를 호출해 처리. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void RecruitCompanion(TSubclassOf<UJobComponent> JobComponent);

	/** 키보드(WASD) Enhanced Input(IA_Move)에서 호출. 카메라가 고정된 월드축 기준으로 이동
	 *  (컨트롤러 회전을 타지 않아 탑다운 카메라와 항상 일치). Triggered 이벤트에 연결할 것. */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void MoveTopDown(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(왼쪽: 이동)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetMoveInput(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(오른쪽: 조준+공격)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetAimInput(FVector2D Value);

	/** 하체 yaw 오프셋(도). AnimInstance(UCombatAnimInstance)가 매 프레임 읽는다. */
	FORCEINLINE float GetLegYawOffset() const { return LegYawOffset; }

	/** 현재 동료 목록(읽기 전용) — 적 타게팅(AEnemy::TrackingPlayer) 등 외부 조회용. */
	FORCEINLINE const TArray<ACompanion*>& GetCompanions() const { return Companions; }

	/** Returns TopDownBoom subobject */
	FORCEINLINE USpringArmComponent* GetTopDownBoom() const { return TopDownBoom; }
	/** Returns TopDownCamera subobject */
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }


private:

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private functions (내부 헬퍼 — 외부/BP 직접 호출 없음)
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 게임패드 아날로그 축 콜백
	void OnMoveX(float Value);
	void OnMoveY(float Value);
	void OnAimX(float Value);
	void OnAimY(float Value);

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

	void CreateTouchJoysticks();

	// 조이스틱 델리게이트 콜백 (→ SetMoveInput / SetAimInput 로 연결)
	UFUNCTION()
	void OnMoveJoystickMoved(FVector2D Value);

	UFUNCTION()
	void OnAimJoystickMoved(FVector2D Value);

	// 초기 셋업 / 내부 헬퍼 (BeginPlay 등 내부에서만 호출)
	void OnTopDownMode();
	void ReplaceWeapon();
	void SetJob();
	void SetMoney(int Money);

	/** 경험치 HUD(텍스트/바) 갱신. AddExp와 SetCanvasWidget(초기 표시)에서 호출. */
	void UpdateExpUI();


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private state (순수 내부 상태 — BP/에디터 노출 없음)
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// UObject 포인터 멤버에는 반드시 UPROPERTY()를 붙인다 — 에디터 노출이 아니라 GC 추적이 목적.
	// 없으면 GC가 "아무도 참조 안 하네" 하고 회수해버려 댕글링 포인터가 되고,
	// if (포인터) 검사는 null이 아닌 쓰레기 주소라 통과해버려 그 안에서 크래시가 난다.
	// (UMyCanvas::UpdateCoinText EXCEPTION_ACCESS_VIOLATION의 원인)

	UPROPERTY()
	UMyCanvas* CanvasWidget = nullptr;

	// 이름을 controller로 두면 안 된다 — APawn에 이미 Controller가 있고,
	// UHT는 프로퍼티 이름을 FName(대소문자 구분 안 함)으로 비교해 shadowing 에러를 낸다.
	UPROPERTY()
	APlayerController* PlayerControllerRef = nullptr;

	UPROPERTY()
	AActor* playerStart = nullptr;

	// 입력 누적값 (게임패드 / 터치를 분리 저장 후 Tick에서 합성)
	FVector2D GamepadMove = FVector2D::ZeroVector;
	FVector2D GamepadAim = FVector2D::ZeroVector;
	FVector2D TouchMove = FVector2D::ZeroVector;
	FVector2D TouchAim = FVector2D::ZeroVector;

	// 공격 타이머(TimeSinceLastAttack)는 베이스(ACombatCharacter::TickAttack)가 관리한다.

	bool bLeftMouseHeld = false;
	bool bRightMouseHeld = false;

	// 직전 유효 커서 방향(월드, 수평). 커서 변환이 실패한 프레임에 이걸 재사용해 끊김 방지.
	FVector LastCursorDir = FVector::ForwardVector;


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Components (private + AllowPrivateAccess — 게터는 public)
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 비스듬한 탑다운 카메라 암 (BP의 기존 CameraBoom과 이름 충돌 방지)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* TopDownBoom;

	// 탑다운 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCamera;

	// NavMesh를 플레이어 주변에만 동적으로 생성시키는 인보커 (무한 맵용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Navigation", meta = (AllowPrivateAccess = "true"))
	UNavigationInvokerComponent* NavInvoker;

	// C++에서 생성하는 터치 조이스틱 인스턴스
	UPROPERTY()
	UVirtualJoystick* MoveJoystick;

	UPROPERTY()
	UVirtualJoystick* AimJoystick;


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Config / BP-exposed (전에 public이었음 — AllowPrivateAccess로 BP·에디터 접근 유지)
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 블루프린트에서 읽고 쓸 수 있는 Money 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Money;
	// HP / Damage 는 베이스(ACombatCharacter)로 이동.

	//////////////////////////////////////////////////////////////////////////
	// 경험치/레벨 — 적을 잡으면 AEnemy::OnDeath가 AddExp를 호출해 쌓인다.

	/** 현재 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;

	/** 현재 레벨에서 쌓은 경험치(레벨업 시 필요량을 빼고 이월) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 Exp = 0;

	/** 1레벨 → 2레벨에 필요한 경험치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 ExpBase = 10;

	/** 레벨당 필요 경험치 증가량 (필요량 = ExpBase + (Level-1) × ExpGrowth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats", meta = (AllowPrivateAccess = "true"))
	int32 ExpGrowth = 5;

	//////////////////////////////////////////////////////////////////////////
	// 무기 강화 — 강화 발판(AWeaponUpgradeZone)에 돈을 넣으면 UpgradeWeapon()이 호출된다.
	// 데미지는 직업 컴포넌트(CurrentJob->Damage) 소유라 강화도 거기에 얹는다.

	/** 현재 무기 강화 레벨(+1 = 1강). 표시용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 WeaponLevel = 0;

	/** 강화 1회당 직업 Damage 증가량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 WeaponDamagePerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	USoundBase* AttackSound; //MS

	//////////////////////////////////////////////////////////////////////////
	// 탑다운 트윈스틱 설정 / 입력

	/** 카메라 내려보는 각도(피치). 음수일수록 위에서 내려봄 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraPitch = -35.0f;

	/** 카메라 거리(암 길이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraDistance = 900.0f;

	/** 스틱 입력 데드존 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Input", meta = (AllowPrivateAccess = "true"))
	float InputDeadzone = 0.25f;

	/** 마우스 이동: 커서가 캐릭터로부터 이 거리(cm) 안이면 이동 정지 — 가까울 때 방향이 뒤집혀 제자리 진동하는 것 방지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Input", meta = (AllowPrivateAccess = "true"))
	float CursorStopRadius = 60.0f;

	/** 켜면 화면에 이동 입력 크기 / 실제 속도 / MaxWalkSpeed를 출력한다. 이동 속도 튜닝용. 기본 꺼짐. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowSpeedDebug = false;

	/** 캐릭터가 조준/이동 방향으로 회전하는 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Movement", meta = (AllowPrivateAccess = "true"))
	float TurnInterpSpeed = 12.0f;

	//////////////////////////////////////////////////////////////////////////
	// 하체/상체 분리 애니메이션 — "다리는 이동 방향, 상체는 조준 방향"
	// 액터(몸)는 조준 방향을 보므로 상체/무기/발사는 그대로 둔다. 다리만 이동 방향으로 돌리려고
	// AnimBP에서 pelvis를 +LegYawOffset, spine_01을 -LegYawOffset 만큼 회전시킨다(상체는 원위치 복귀).
	// → 전방 워크 애니메이션 1개만 있어도 옆/뒤로 걸을 때 다리가 이동 방향을 향한다.

	/** 다리를 이동 방향으로 돌리기 위한 yaw 오프셋(도). 액터 정면(=조준) 기준 이동 방향과의 각도. AnimBP가 읽는다. */
	UPROPERTY(BlueprintReadOnly, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawOffset = 0.0f;

	/** LegYawOffset 보간 속도(클수록 다리가 이동 방향으로 빨리 돌아감) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawInterpSpeed = 10.0f;

	/** LegYawOffset 최대 각도(도). 전방 애니 1개라 이 이상은 허리가 부러져 보여서 막는다(보통 90). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Animation", meta = (AllowPrivateAccess = "true"))
	float LegYawMaxAngle = 90.0f;

	// 자동 공격 간격(AttackInterval)·직업(DefaultJobClass/CurrentJob)·공격 몽타주(AttackMontage)는
	// 모두 베이스(ACombatCharacter)로 이동했다. 동료(ACompanion)와 똑같은 배선이라 한곳에 모음.

	//////////////////////////////////////////////////////////////////////////
	// 무기 메시 교체 — 캐릭터(BP)에 이미 붙어 있는 무기 컴포넌트의 메시를 직업에 맞게 바꾼다.

	/** 무기 ChildActorComponent(예: Weapon_BP)를 고를 때 쓰는 태그. 여러 개일 때 그 컴포넌트 Details에서 달면 우선 선택됨.
	 *  하나뿐이면 태그 없어도 자동으로 잡힌다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FName WeaponComponentTag = TEXT("Weapon");

	/** 무기 ChildActor(BP_sword 등) 안의 메시 컴포넌트. BeginPlay에서 탐색해 캐시. 직업이 이 메시를 교체한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMeshComponent = nullptr;

	//////////////////////////////////////////////////////////////////////////
	// 동료 섭외 — 동료를 스폰해 플레이어를 따라다니며 싸우게 한다.

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

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Debug", meta = (AllowPrivateAccess = "true"))
	//bool bDebugRecruitKey = true;

	/** true면 C 키로 디버그용 돈 획득(AddMoney)을 테스트할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugAddMoneyKey = true;

	//////////////////////////////////////////////////////////////////////////
	// 터치 조이스틱 (C++가 자동 생성해서 화면에 띄움 — 위젯 BP 불필요)

	/** (선택) 조이스틱 베이스 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Touch", meta = (AllowPrivateAccess = "true"))
	UTexture2D* JoystickBackgroundTexture = nullptr;

	/** (선택) 조이스틱 손잡이 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Touch", meta = (AllowPrivateAccess = "true"))
	UTexture2D* JoystickHandleTexture = nullptr;


	bool CheckCompanion(UWorld* World);
	FTransform SetSpawnTransformCompanion(UWorld* World);
};
