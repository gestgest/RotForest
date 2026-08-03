// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Jobs/JobComponent.h"
#include "CombatCharacter.generated.h"

class UAnimMontage;
struct FBranchingPointNotifyPayload;
class UWidgetComponent;
class UChildActorComponent;

/** 무기 슬롯 전용 로그 카테고리.
 *  출력 로그 창 → Categories 드롭다운에서 'LogWeapon'만 체크하면 무기 관련 로그만 볼 수 있다.
 *  (LogTemp에 섞어 쓰면 다른 노란 경고에 묻힌다) */
DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Log, All);


UENUM(BlueprintType)
enum class ETeam : uint8
{
	Ally     UMETA(DisplayName = "아군"),   // 플레이어, 동료
	Enemy    UMETA(DisplayName = "적"),
	Neutral  UMETA(DisplayName = "중립"),  
};

UCLASS(Abstract)
class ZOMBIEHUNTER1_API ACombatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	void ApplyJobStats(FJobStats Stats);
	
	// 공격 몽타주를 반환. => 대체로 공격할때 호출
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UAnimMontage* GetAttackMontageForJob(EJobType JobType) const;



	/////////////////////////////////////////////////////////////////////////////////////////
	// 직업(Job)
	//  — 플레이어/동료가 공유한다. 직업이 없는 캐릭터(적)는 그냥 비워두면 된다.
	/////////////////////////////////////////////////////////////////////////////////////////

	/** BeginPlay에서 호출. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void CreateJobComponent();

	/** 실제 적용할 자동 공격 간격(초). 직업 값이 유효하면 그것, 아니면 아래 AttackInterval 폴백. */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetAttackInterval() const;

	/** 공격 판단 쿨타임 함수 */
	bool TickAttack(float DeltaTime, bool bWantsToAttack);

	
	
	// 시작 시 부착할 직업 클래스. 비우면 직업 없이 동작한다(적 등). 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> DefaultJobClass;


	// 무장 상태 
	// UCombatAnimInstance가 매 프레임 이 값을 bArmed 변수로 미러링해 AnimBP에 공급한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bArmed = true;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	ETeam TeamType;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;






	// 반드시 서브클래스 생성자에서만 호출할 것 => Companion 같은 생성자
	// 컴포넌트 이름은 "HPBar" 
	void CreateHPBarComponent();

	// SetHP가 호출.
	void UpdateHPBar();

	/** HP 변화에 따라 IsDead를 바꾸고 OnDeath/OnRevive 훅을 전환 시점에 1회씩 호출. */
	void SetDead(bool bNewDead);


	//초반에 호출
	void InitWeaponSlot();

	/** 살아있음 → 죽음 전환 시 1회. 서브클래스가 AI 정지/콜리전 해제/연출 등을 구현. */
	virtual void OnDeath() {}

	/** 죽음 → 부활(풀 재사용 등) 전환 시 1회. 죽을 때 껐던 것들을 되돌린다. */
	virtual void OnRevive() {}



	/** 공격 몽타주 Notify가 들어왔을 때 호출.
	 *  기본 구현은 현재 직업의 OnAttackNotify()로 넘긴다(플레이어/동료가 쓰던 동작).
	 *  직업이 없는 캐릭터(AEnemy)만 이걸 재정의해 자기 방식으로 타격한다. */
	virtual void HandleAttackNotify(FName NotifyName);


	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);





	//////////////////////////////////////////////////////////////////////////////////////////
	//Stats
	//////////////////////////////////////////////////////////////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxHP = 5;

	// SetHP로만 소통하자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 HP = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Damage = 1;

	/** 죽었는지 여부. HP<=0이면 true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool IsDead = false;



	/** 무기 슬롯이 스폰할 액터 클래스(Weapon_BP). 양손 슬롯이 같은 클래스를 쓴다 —
	 *  무기 액터는 빈 껍데기이고, 안의 메시는 직업이 런타임에 갈아끼우기 때문. 캐릭터 BP에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> WeaponActorClass;

	/** 오른손 무기가 붙을 스켈레톤 소켓 이름. 캐릭터마다 스켈레톤이 다르면 BP에서 바꾼다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName RightHandSocket = TEXT("weapon_r");

	/** 왼손 무기가 붙을 스켈레톤 소켓 이름(활처럼 왼손으로 드는 무기용). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName LeftHandSocket = TEXT("weapon_l");



	/** JobAttackMontages에 없다면 호출되는 디폴트 공격 몽타주.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* DefaultAttackMontage = nullptr;

	/** 직업별 공격 몽타주 => 몽타주[키] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TMap<EJobType, UAnimMontage*> JobAttackMontages;

	// 자동 공격 간격(초) => 직업이 없는 Enemy은 이거를 참조한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float AttackInterval = 0.4f;





	/** 현재 부착된 직업 컴포넌트(런타임 생성). 직업 없는 캐릭터는 null. */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	UJobComponent* CurrentJob = nullptr;




	/** 오른손 무기 슬롯 — 생성자에서 만들어 RightHandSocket에 붙는다. BP에서 추가할 필요 없음.
	 *  ★ 무기를 잡는 각도/위치는 이 컴포넌트의 Relative Rotation/Location으로 맞춘다.
	 *    (소켓이 아니라 여기서 맞추는 이유: 부착이 KeepRelativeTransform이라 이 값이 그대로 유지된다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UChildActorComponent* WeaponRight = nullptr;

	/** 왼손 무기 슬롯 — 생성자에서 만들어 LeftHandSocket에 붙는다. 각도 조정은 WeaponRight와 동일. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UChildActorComponent* WeaponLeft = nullptr;

	/** 각 슬롯의 Weapon_BP 안에 있는 스켈레탈 메시 컴포넌트. BeginPlay(InitWeaponSlot)에서 캐시. */
	UPROPERTY()
	USkeletalMeshComponent* WeaponRightMesh = nullptr;

	UPROPERTY()
	USkeletalMeshComponent* WeaponLeftMesh = nullptr;

	/** 지금 장착 중인 손의 메시 컴포넌트. EquipWeaponInHand가 갱신한다.
	 *  (궁수의 활 시위 애니처럼) 무기 메시를 직접 만져야 하는 직업이 이걸 집어간다. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMeshComponent = nullptr;





	//마지막 공격 이후 누적 시간(TickAttack이 관리).
	float TimeSinceLastAttack = 0.0f;




	/** 머리 위 HP 바(스크린 스페이스). 서브클래스 "생성자"가 CreateHPBarComponent()를 불러야 생긴다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HPBarComponent = nullptr;



	/////////////////////////////////////////////////////////////////////////////////////////
	//디버깅 
	/////////////////////////////////////////////////////////////////////////////////////////
	
	// 전투 캐릭터의 상태/공격 디버그를 화면에 그린다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
	bool bDebugCombat = false;



private:
	/** 메시 애님 인스턴스의 OnPlayMontageNotifyBegin에 바인딩 → HandleAttackNotify로 전달. */
	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);


public: 
	//property
	//여기서 호출하면 인라인화된다.
	int32 GetMaxHP() { return MaxHP; }
	int32 GetHP() { return HP; }
	int32 GetDamage() { return Damage; }
	bool GetIsDead() { return IsDead; }

	USkeletalMeshComponent* GetWeaponMeshComponent() const { return WeaponMeshComponent; }

	/** 죽음 상태, HP바 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetHP(int32 new_hp);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AddHP(int32 add_hp);


	/** Hand 쪽 손에 무기 메시를 끼우고 반대 손은 숨긴다. JobComponent가 호출.
	 *  NewMesh가 null이면 양손 다 숨긴다(무기 없는 직업). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponInHand(USkeletalMesh* NewMesh, EWeaponHand Hand);

	// 오른손에 끼우는 단축형.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(USkeletalMesh* NewMesh);
};

/**
 * 전투 캐릭터 공통 베이스 — 플레이어(AMyPlayer)/동료(ACompanion)/적(AEnemy)이 공유한다.
 * 셋의 공통분모인 체력(HP)·데미지·죽음/부활 전환·공격 몽타주 Notify 배선을 한곳에 모은다.
 *
 * 실제 공격 처리(직업 호출 or 자체 타격)와 죽음 연출은 서브클래스가 가상 함수로 구현한다:
 *  - HandleAttackNotify() : 공격 몽타주의 Notify가 들어왔을 때 실제 타격을 어떻게 줄지
 *  - OnDeath()/OnRevive() : 죽거나(전환) 풀에서 되살아날 때 무엇을 끄고/켤지
 */
