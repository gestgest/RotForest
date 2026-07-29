// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Jobs/JobComponent.h"
#include "CombatCharacter.generated.h"

class UAnimMontage;
struct FBranchingPointNotifyPayload;
class UWidgetComponent;

/**
 * 전투 캐릭터 공통 베이스 — 플레이어(AMyPlayer)/동료(ACompanion)/적(AEnemy)이 공유한다.
 * 셋의 공통분모인 체력(HP)·데미지·죽음/부활 전환·공격 몽타주 Notify 배선을 한곳에 모은다.
 *
 * 실제 공격 처리(직업 호출 or 자체 타격)와 죽음 연출은 서브클래스가 가상 함수로 구현한다:
 *  - HandleAttackNotify() : 공격 몽타주의 Notify가 들어왔을 때 실제 타격을 어떻게 줄지
 *  - OnDeath()/OnRevive() : 죽거나(전환) 풀에서 되살아날 때 무엇을 끄고/켤지
 */
UCLASS(Abstract)
class ZOMBIEHUNTER1_API ACombatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACombatCharacter();


	//Stats
	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxHP = 5;

	/** 현재 체력. 0 이하가 되면 SetHP가 자동으로 IsDead=true로 만든다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 HP = 5;

	/** 한 번 공격 시 주는 데미지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Damage = 1;

	/** 죽었는지 여부. HP<=0이면 true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool IsDead = false;






	/** 무장 상태 — idle/walk 블렌드스페이스(무장/비무장) 선택에 쓰인다.
	 *  UCombatAnimInstance가 매 프레임 이 값을 bArmed 변수로 미러링해 AnimBP에 공급한다.
	 *  플레이어/동료 공용. 동료는 항상 무장(기본 true). 무기 넣/뺌 연출을 넣고 싶으면 이 값을 토글하면 된다.
	 *  (BP에선 앞의 b가 빠진 "Armed"로 표시됨) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bArmed = true;

	/** JobAttackMontages에 없다면 호출되는 공격 몽타주. 거의 안쓰인다.
	Notify가 HandleAttackNotify()를 호출한다(직업 없는 적 + 맵에 없을 때 폴백용). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* AttackMontage = nullptr;

	/** 직업별 공격 몽타주 — 이 캐릭터의 "스켈레톤에 맞게" 리타게팅된 몽타주를 직업 이름으로 매핑한다.
	 *  애니는 스켈레톤에 묶이므로 직업(공유 컴포넌트)이 아니라 캐릭터가 소유해야 한다.
	 *  키 = 직업의 enum 비우거나 키가 없으면 AttackMontage로 폴백. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TMap<EJobType, UAnimMontage*> JobAttackMontages;

	/** JobName에 해당하는 이 캐릭터의 공격 몽타주를 반환. 맵에 없으면 단일 AttackMontage로 폴백(없으면 null). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UAnimMontage* GetAttackMontageForJob(EJobType JobType) const;

	/////////////////////////////////////////////////////////////////////////
	// 직업(Job) — 플레이어/동료가 공유한다. 직업이 없는 캐릭터(적)는 그냥 비워두면 된다.
	// "무엇을 언제 칠지"는 서브클래스가 정하고(입력/AI), "어떻게 치는지"는 직업이 정한다.

	/** 시작 시 부착할 직업 클래스. 비우면 직업 없이 동작한다(적 등). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> DefaultJobClass;

	/** 현재 부착된 직업 컴포넌트(런타임 생성). 직업 없는 캐릭터는 null. */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	UJobComponent* CurrentJob = nullptr;

	/** DefaultJobClass로 직업 컴포넌트를 만들어 부착하고 스탯을 적용한다. 서브클래스가 BeginPlay에서 호출.
	 *  이미 직업이 있으면 떼어내고 교체하므로, 직업 변경에도 그대로 쓸 수 있다. */
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void CreateJobComponent();

	/** 실제 적용할 자동 공격 간격(초). 직업 값이 유효하면 그것, 아니면 아래 AttackInterval 폴백. */
	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetAttackInterval() const;

	/** 자동 공격 타이머를 굴린다. 매 프레임 호출할 것.
	 *  bWantsToAttack이 true이고 간격이 찼으면 현재 직업의 Attack()을 1회 실행하고 true를 반환한다.
	 *  "공격하고 싶은가"의 판단(플레이어=조준 입력, 동료=사거리 안의 타겟)만 서브클래스가 하면 된다. */
	bool TickAttack(float DeltaTime, bool bWantsToAttack);

	/** 자동 공격 간격(초) 폴백 — 직업이 없거나 직업의 Stats.AttackInterval이 0 이하일 때 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float AttackInterval = 0.4f;

	/** true면 이 전투 캐릭터의 상태/공격 디버그를 화면에 그린다. 플레이어/동료/적 공용 토글. 기본 꺼짐. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
	bool bDebugCombat = false;

	// 파라미터 이름(add_hp/new_hp)은 기존 AMyPlayer 버전과 동일하게 유지한다.
	// BP는 핀을 C++ 파라미터 이름(내부 FName)으로 저장하므로, 바꾸면 기존 BP 연결이 "핀 못 찾음"으로 깨진다.

	/** HP를 add_hp만큼 증감(데미지는 음수, 회복은 양수). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AddHP(int32 add_hp);

	/** HP를 설정하고 죽음/부활 전환을 갱신한다. 머리 위 HP 바가 있으면(적/동료) 같이 갱신. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetHP(int32 new_hp);

	/** 머리 위 HP 바(스크린 스페이스). 서브클래스 "생성자"가 CreateHPBarComponent()를 불러야 생긴다.
	 *  적/동료만 만들고, 플레이어는 안 만든다(HUD 체력바 사용) — null이면 SetHP가 그냥 건너뛴다.
	 *  표시할 위젯 클래스(WBP_EnemyHPBar)는 각 BP에서 이 컴포넌트에 지정. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HPBarComponent = nullptr;

protected:
	virtual void BeginPlay() override;

	/** 머리 위 HP 바 컴포넌트를 생성한다. 반드시 서브클래스 생성자에서만 호출할 것(CreateDefaultSubobject 제약).
	 *  컴포넌트 이름은 "HPBar" — BP에 저장된 위젯 클래스 지정이 이름으로 매칭되므로 바꾸면 안 된다. */
	void CreateHPBarComponent();

	/** HP/MaxHP 비율로 바를 채우고, 죽으면 숨긴다. SetHP가 호출. */
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

	/** 마지막 공격 이후 누적 시간(TickAttack이 관리). */
	float TimeSinceLastAttack = 0.0f;

private:
	/** 메시 애님 인스턴스의 OnPlayMontageNotifyBegin에 바인딩 → HandleAttackNotify로 전달. */
	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
};
