// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Jobs/JobComponent.h"
#include "Items/WeaponItemData.h"
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

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public: //핵심 함수
	// [Begin]
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void CreateJobComponent();

	// [Init]
	// 직업에 맞는 능력 부여. => JobComponent에서 호출
	void ApplyJobStats(FJobStats Stats);

	// [공격]
	// 공격 몽타주를 반환. => 대체로 공격할때 호출
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UAnimMontage* GetAttackMontageForJob(EJobType JobType) const;


protected: //핵심 함수의 부품

	// [생성자]
	// 반드시 서브클래스 생성자에서만 호출할 것 => Companion 같은 생성자
	// ㄴ MyPlayer는 따로 UI가 있다.
	void CreateHPBarComponent();

	// [Begin]
	void InitWeaponSlot();

	
	
	// SetHP가 호출.
	void UpdateHPBar();

	/** HP 변화에 따라 IsDead를 바꾸고 OnDeath/OnRevive 훅을 전환 시점에 1회씩 호출. */
	void SetDead(bool bNewDead);



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





	// 현재 부착된 직업 컴포넌트(런타임 생성). 직업 없는 캐릭터는 null.
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

	// 지금 장착한 무기 데이터. 장비는 캐릭터가 소유한다 — 직업이 바뀌어도 들고 있던 무기는 남는다.
	// Transient — 한 판 동안만 유효한 값이라 애셋/세이브에 굳으면 안 된다(BonusDamage와 같은 성격).
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	FWeaponItemData EquippedWeapon;

	/** 각 슬롯의 Weapon_BP 안에 있는 스켈레탈 메시 컴포넌트. BeginPlay(InitWeaponSlot)에서 캐시. */
	UPROPERTY()
	USkeletalMeshComponent* WeaponRightMesh = nullptr;

	UPROPERTY()
	USkeletalMeshComponent* WeaponLeftMesh = nullptr;

	/** 지금 장착 중인 손의 메시 컴포넌트. EquipWeaponInHand가 갱신한다.
	 *  (궁수의 활 시위 애니처럼) 무기 메시를 직접 만져야 하는 직업이 이걸 집어간다. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMeshComponent = nullptr;




	// 머리 위 HP 바(스크린 스페이스). 서브클래스 "생성자"가 CreateHPBarComponent()를 불러야 생긴다.
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

protected:
	// SetHP로만 소통하자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 HP = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Damage = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxHP = 5;

	/** 죽었는지 여부. HP<=0이면 true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool IsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackRange = 100.0f;


	// [공격 시간]
	//마지막 공격 이후 누적 시간(TickAttack이 관리).
	float TimeSinceLastAttack = 0.0f;

	//공격 시작 순간의 정면 방향(TickAttack이 고정).
	FVector AttackAimDir = FVector::ZeroVector;


public: //Property
	//여기서 호출하면 인라인화된다.
	int32 GetMaxHP() { return MaxHP; }
	int32 GetHP() { return HP; }
	int32 GetDamage() { return Damage; }
	bool GetIsDead() { return IsDead; }

	UJobComponent* GetJobComponent() const { return CurrentJob; }
	USkeletalMeshComponent* GetWeaponMeshComponent() const { return WeaponMeshComponent; }

	// 죽음 상태, HP바 갱신 
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

	// 무기 데이터를 갈아끼우고 손 슬롯 반영까지 한다.
	// 무기 획득처(픽업/상점/보상)는 전부 이 함수 하나로 들어온다.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeaponItem(const FWeaponItemData& Item);

	// 이 무기를 지금 것과 바꿀 가치가 있는지. 직업이 맞고 공격력이 더 높아야 한다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool WantsWeaponItem(const FWeaponItemData& Item) const;

	// 지금 장착 무기의 메시를 직업이 지정한 손에 다시 끼운다. 데이터는 건드리지 않는다.
	void RefreshWeaponMesh();

	const FWeaponItemData& GetEquippedWeapon() const { return EquippedWeapon; }




	// 직업(Job)
	// 실제 적용할 자동 공격 간격(초). 직업 값이 유효하면 그것, 아니면 아래 AttackInterval 폴백. 
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetAttackInterval() const;

	// 공격 판단 쿨타임 함수 
	bool TickAttack(float DeltaTime, bool bWantsToAttack);

	// 공격을 시작한 순간에 고정해 둔 공격 방향(월드, 수평).
	// 타격 프레임(Notify)엔 몸이 이미 다른 데를 보고 있을 수 있어 이 값으로 때린다.
	FVector GetAttackAimDir() const;



	// 시작 시 부착할 직업 클래스. 비우면 직업 없이 동작한다(적 등). 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> DefaultJobClass;


	// 무장 상태 
	// UCombatAnimInstance가 매 프레임 이 값을 bArmed 변수로 미러링해 AnimBP에 공급한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bArmed = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	ETeam TeamType;

};
