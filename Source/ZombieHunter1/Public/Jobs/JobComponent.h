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
class UJobComponent;

//enum의 E
UENUM(BlueprintType)
enum class EJobType : uint8
{
	Warrior UMETA(DisplayName = "전사"),
	Archer UMETA(DisplayName = "궁수"),
	Mage UMETA(DisplayName = "마법사"),
	Healer UMETA(DisplayName = "힐러"),
};


//무기를 드는 손
UENUM(BlueprintType)
enum class EWeaponHand : uint8
{
	Right UMETA(DisplayName = "오른손"),
	Left  UMETA(DisplayName = "왼손"),
};


USTRUCT(BlueprintType)
struct FWeaponItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Weapon")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Weapon")
	int32 WeaponPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Weapon")
	USkeletalMesh* Mesh = nullptr;

	// 이 무기를 쓸 수 있는 직업. 다른 직업은 장착할 수 없다(검사가 활을 못 듦).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Weapon")
	EJobType JobType = EJobType::Warrior;
};


USTRUCT(BlueprintType)
struct FJobStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 MaxHP = 100; //몰랐네 무조건 int32를 써야하다니

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 Damage = 0;

	//이동 속도(cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	int32 Speed = 600;

	// 자동 공격 간격(초). 0 이하면 캐릭터의 AttackInterval 폴백을 쓴다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Stats")
	float AttackInterval = 0.0f;
};


/**
 * 직업 하나에 딸린 "직업 선택 화면이 알아야 할 것"의 묶음.
 * 스탯(FJobStats)은 각 직업 컴포넌트 CDO가 들고 있으므로 여기 두지 않는다 — 두 벌이 되면 반드시 어긋난다.
 *
 * DisplayName이 게임 UI에 보이는 이름의 "원본"이다.
 * EJobType의 UMETA(DisplayName)은 에디터 디테일 패널용으로 그대로 남겨둔다 — 용도가 다른 두 텍스트다.
 * FString이 아니라 FText인 이유: 나중에 String Table을 물려 다국어로 넘어갈 때 이 시그니처를 안 바꿔도 된다.
 */
USTRUCT(BlueprintType)
struct FJobDefinition
{
	GENERATED_BODY()

	/** 어떤 직업인지. 배열 순서와 무관하게 이 값으로 직업을 식별한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	EJobType JobType = EJobType::Warrior;

	/** 실제 구현 클래스(BP_ArcherJob 등). 선택 UI가 이 값을 GameInstance의 SelectedJobClass로 넘긴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> JobClass;

	/** 플레이어에게 보이는 이름("궁수"). 콤보박스 옵션 텍스트가 여기서 나온다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	FText DisplayName;

	// 아이콘·설명·해금여부가 필요해지면 Map을 새로 만들지 말고 여기에 필드를 추가한다.
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

	// 주운/구매한 무기를 장착한다. 데이터만 바꾸고 손 슬롯 반영은 RefreshWeaponMesh()에 위임한다.
	// 다른 직업 전용 무기는 거부한다.
	// 반환: 장착에 성공하면 true, 직업이 안 맞아 거부하면 false.
	UFUNCTION(BlueprintCallable, Category = "Job|Weapon")
	bool EquipWeapon(const FWeaponItemData& Item);


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




	/** 이 직업이 무기를 드는 손. 활은 왼손으로 들고 오른손으로 시위를 당기므로 궁수는 Left.
	 *  직업 BP에서 바꿀 수 있다(왼손잡이 검사 등). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job|Weapon")
	EWeaponHand WeaponHand = EWeaponHand::Right;


	// 지금 장착 무기(EquippedWeapon)의 메시를 WeaponHand 쪽 슬롯에 꽂는다.
	// 데이터는 건드리지 않는다 — 상태 변경은 EquipWeapon()이 하고 여기는 반영만 한다.
	void RefreshWeaponMesh();

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


	// 직업 기본 무기 — 직업 BP(BP_ArcherJob 등)에서 지정한다. 시작 시 EquippedWeapon으로 복사되고 그 뒤로 안 변한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Job|Weapon")
	FWeaponItemData DefaultWeapon;

	// 지금 장착 중인 무기. InitializeForOwner에서 DefaultWeapon으로 채워지므로 항상 유효하다.
	// Transient — 한 판 동안만 유효한 값이라 애셋/세이브에 굳으면 안 된다(BonusDamage와 같은 성격).
	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Job|Weapon")
	FWeaponItemData EquippedWeapon;


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

	// 실제 적용 피해량 = 직업 설계값 + 강화/버프 증가분. 공격 코드는 전부 이걸 쓴다. 
	UFUNCTION(BlueprintPure, Category = "Job|Stats")
	int32 GetDamage() const { return Stats.Damage + BonusDamage + EquippedWeapon.WeaponPower; }

	USkeletalMesh* GetWeaponMesh() const { return EquippedWeapon.Mesh; }

	// 이 직업의 기본 무기 메시. CDO에 물어볼 때 쓴다(동료 스폰존 아이콘 등) —
	// CDO는 InitializeForOwner를 안 타므로 EquippedWeapon이 비어 있다.
	USkeletalMesh* GetDefaultWeaponMesh() const { return DefaultWeapon.Mesh; }
	EWeaponHand GetWeaponHand() const { return WeaponHand; }
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
