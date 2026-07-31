// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Jobs/JobComponent.h"
#include "CombatCharacter.generated.h"

class UAnimMontage;
struct FBranchingPointNotifyPayload;
class UWidgetComponent;

UCLASS(Abstract)
class ZOMBIEHUNTER1_API ACombatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	void ApplyJobStats(FJobStats Stats);
	

	//////////////////////////////////////////////////////////////////////////////////////////
	//Combat
	//////////////////////////////////////////////////////////////////////////////////////////

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
	


protected:
	virtual void BeginPlay() override;

	// 반드시 서브클래스 생성자에서만 호출할 것 => Companion 같은 생성자
	// 컴포넌트 이름은 "HPBar" 
	void CreateHPBarComponent();

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




	/** 무기 ChildActor(BP_sword 등) 안의 메시 컴포넌트. BeginPlay에서 탐색해 캐시. 직업이 이 메시를 교체한다. */
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


	/** 죽음 상태, HP바 갱신 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetHP(int32 new_hp);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AddHP(int32 add_hp);


	// 무기 컴포넌트의 스켈레탈 메시를 교체한다. NewMesh가 null이면 무기를 숨긴다. JobComponent이 호출. 
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponMesh(USkeletalMesh* NewMesh);
};

/**
 * 전투 캐릭터 공통 베이스 — 플레이어(AMyPlayer)/동료(ACompanion)/적(AEnemy)이 공유한다.
 * 셋의 공통분모인 체력(HP)·데미지·죽음/부활 전환·공격 몽타주 Notify 배선을 한곳에 모은다.
 *
 * 실제 공격 처리(직업 호출 or 자체 타격)와 죽음 연출은 서브클래스가 가상 함수로 구현한다:
 *  - HandleAttackNotify() : 공격 몽타주의 Notify가 들어왔을 때 실제 타격을 어떻게 줄지
 *  - OnDeath()/OnRevive() : 죽거나(전환) 풀에서 되살아날 때 무엇을 끄고/켤지
 */
