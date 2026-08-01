// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JobComponent.generated.h"

class ACombatCharacter;
class UAnimMontage;
class USoundBase;
class AProjectile;
class USkeletalMesh;

//enum의 E
UENUM(BlueprintType)
enum class EJobType : uint8
{
	Warrior UMETA(DisplayName = "전사"),
	Archer UMETA(DisplayName = "궁수"),
	Mage UMETA(DisplayName = "마법사"),
	Healer UMETA(DisplayName = "힐러"),
};




USTRUCT(BlueprintType)
struct FJobStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 MaxHP; //몰랐네 무조건 int32를 써야하다니

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 Damage;

	/** 이동 속도(cm/s) — 캐릭터의 CharacterMovement->MaxWalkSpeed로 적용된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 Speed;

	/** 자동 공격 간격(초). 0 이하면 캐릭터의 AttackInterval 폴백을 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	float AttackInterval;
};




UCLASS(Abstract, Blueprintable, ClassGroup = (Job), meta = (BlueprintSpawnableComponent))
class ZOMBIEHUNTER1_API UJobComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJobComponent();

	// [초반] 소유 캐릭터(플레이어/동료)를 연결한다. 소유자가 컴포넌트 생성 직후 호출.
	void InitializeForOwner(ACombatCharacter* Owner);

	//공격 몽타주(JobAttackMontages)를 재생한다. Joptype을 전달해서 Character 몽타주 실행
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void Attack();


	// 매 프레임 호출(플레이어 Tick). 조준과 무관한 패시브 효과(자가 회복 등)용. 기본 구현 없음. 
	virtual void TickJob(float DeltaTime);


protected:
	/** 소유 캐릭터 — 플레이어 또는 동료 AI (InitializeForOwner에서 설정) */
	UPROPERTY()
	ACombatCharacter* OwnerCharacter = nullptr;


	// 동료 AI 교전 사거리(cm) — 적이 이 거리 안에 들면 멈춰서 공격한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Job|Combat")
	float EngageRange = 500.0f;


	//직업의 공격 판정 범위를 화면에 그린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Job|Debug")
	bool bDebugAttack = false;




	// 따로 지정된 WeaponMesh 무기를 장착한다.
	void EquipWeapon();

	/** 공격 사운드를 소유자 위치에서 재생 */
	void PlayAttackSound();



	/**
	 * 전방으로 발사체를 스폰하고 직업의 Damage/속도를 적용해 반환한다 (궁수/마법사 공용).
	 * @param ProjectileClass  스폰할 발사체 클래스
	 * @param Speed            발사 속도(cm/s)
	 * @param MuzzleOffset     캐릭터 앞쪽으로 스폰하는 거리(cm)
	 * @param MuzzleHeight     스폰 높이 보정(cm)
	 * @return 스폰된 발사체(실패 시 nullptr). 호출 측에서 폭발반경 등 추가 설정 가능.
	 */
	AProjectile* SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight);

private:

	// 공격 적중 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Job|Combat")
	USoundBase* AttackSound = nullptr;

	/** 이 직업이 들 무기 메시(검사=검, 궁수=활 등). 비우면 무기 숨김(예: 지팡이 없는 마법사).
	 *  캐릭터에 이미 있는 무기 컴포넌트의 메시를 이걸로 교체한다. 직업 BP 서브클래스에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Job|Weapon")
	USkeletalMesh* WeaponMesh = nullptr;




public:

	// 공격 몽타주의 Notify에서 호출되는 실제 타격 판정. 기본 구현은 비어 있음. 
	UFUNCTION(BlueprintCallable, Category = "Job")
	virtual void OnAttackNotify(FName NotifyName);

	// 공격 몽타주가 끝까지 재생됐을 때. 기본 구현 없음.
	virtual void OnAttackMontageEnded() {}

	////////////////////////////////////////////////////////////////////////
	//Variables
	// 직업 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job")
	EJobType JobType = EJobType::Warrior;

	// 직업 상태
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job")
	FJobStats Stats;

	// 추가 데미지 => 게임이 끝나면 사라져야 하는 값
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Job|Combat")
	int32 BonusDamage = 0;

	// 실제 적용 피해량 = 직업 설계값 + 강화/버프 증가분. 공격 코드는 전부 이걸 쓴다. */
	UFUNCTION(BlueprintPure, Category = "Job|Stats")
	int32 GetDamage() const { return Stats.Damage + BonusDamage; }

	USkeletalMesh* GetWeaponMesh() { return WeaponMesh; }
	float GetEngageRange() { return EngageRange; }

	USoundBase* GetAttackSound() { return AttackSound; }
	void SetAttackSound(USoundBase* Value) { AttackSound = Value; }
};



/**
 * 직업(Job) 베이스 컴포넌트.
 * 이동/조준은 소유 캐릭터(플레이어 또는 동료 AI)가 담당하고, "공격 방식"만 이 컴포넌트로 분리한다.
 * 소유자는 ACharacter이면 무엇이든 가능 — 플레이어(AMyPlayer)와 동료(ACompanion)가 같은 직업을 공유한다.
 * 직업마다 서브클래스(UWarriorJob, UArcherJob...)에서 Attack()/OnAttackNotify()를 재정의한다.
 *
 * - Attack()        : 자동공격 타이밍에 플레이어가 호출. 기본 동작은 소유 캐릭터의 직업별 공격 몽타주 재생.
 * - OnAttackNotify(): 공격 몽타주의 Notify 시점에 플레이어가 호출. 실제 피해/발사 판정을 여기서.
 */
