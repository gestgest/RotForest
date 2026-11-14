// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/JobComponent.h"

#include "Characters/Enemy.h"
#include "Characters/MyPlayer.h"
#include "Characters/CombatCharacter.h"

#include "Projectiles/Projectile.h"
#include "Projectiles/ProjectilePoolSubsystem.h"

#include "Jobs/SkillDataAsset.h"

#include "Items/ItemDataAsset.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"

#include "Kismet/GameplayStatics.h"

#include "GameFramework/CharacterMovementComponent.h"


UJobComponent::UJobComponent()
{
	// 직업 컴포넌트는 Tick이 필요 없다 (공격은 플레이어가 타이밍을 호출).
	PrimaryComponentTick.bCanEverTick = false;
}

//참조하는 곳 ACombatCharacter::CreateJobComponent()
void UJobComponent::InitializeForOwner(ACombatCharacter* Owner)
{
	OwnerCharacter = Owner;
	if (!OwnerCharacter)
		return;

	//값 전송
	OwnerCharacter->ApplyJobStats(Stats);

	if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Stats.Speed;
	}

	//OwnerCharacter->AttackInterval = Stats.AttackInterval; => 어차피 계속 Get으로 받을 예정

	// 직업 기본 무기를 캐릭터에 장착시킨다. 이 뒤로 "지금 무기"는 캐릭터가 든 것 하나뿐이다.
	OwnerCharacter->EquipWeaponItem(DefaultWeapon);
}

void UJobComponent::Attack()
{
	// 애니(몽타주)는 캐릭터가 자기 스켈레톤에 맞게 소유한다. 직업은 자기 JobName으로 골라 재생만 시킨다.
	// 몽타주의 Notify는 캐릭터 베이스(ACombatCharacter)가 받아 OnAttackNotify()로 되돌려준다.
	if (ACombatCharacter* CC = Cast<ACombatCharacter>(OwnerCharacter))
	{
		if (UAnimMontage* Montage = CC->GetAttackMontageForJob(JobType))
		{
			CC->PlayAnimMontage(Montage);
		}
	}
}

bool UJobComponent::TryStartSkill()
{
	if (!CanUseSkill())
	{
		return false;
	}

	if (SkillData->Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SkillData->Sound, OwnerCharacter->GetActorLocation());
	}

	UseSkill();
	
	// 즉발은 바로 쿨타임
	if (!bSkillActive)
	{
		StartSkillCooldown();
	}
	return true;
}

// 무기는 캐릭터가 소유하므로 소유자가 없으면(CDO 등) 무기분은 빠진다.
int32 UJobComponent::GetDamage() const
{
	const int32 WeaponPower = OwnerCharacter ? OwnerCharacter->GetEquippedWeaponPower() : 0;

	return Stats.Damage + BonusWeaponDamage + WeaponPower + BonusLevelDamage;
}

USkeletalMesh* UJobComponent::GetDefaultWeaponMesh() const
{
	return DefaultWeapon ? DefaultWeapon->Mesh : nullptr;
}

void UJobComponent::OnAttackNotify(FName NotifyName)
{
	// 기본 구현 없음. 직업별 서브클래스에서 재정의한다.
}

void UJobComponent::TickJob(float DeltaTime)
{
	// 기본 구현 없음. 패시브가 필요한 직업(힐러 등)에서 재정의한다.
}

void UJobComponent::PlayAttackSound()
{
	if (OwnerCharacter && AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			OwnerCharacter,
			AttackSound,
			OwnerCharacter->GetActorLocation(),
			1.0f,
			FMath::FRandRange(0.9f, 1.10f)
		);
	}
}


AProjectile* UJobComponent::SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight)
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	return SpawnProjectile(ProjectileClass, OwnerCharacter->GetAttackAimDir(), Speed, MuzzleOffset, MuzzleHeight);
}


AProjectile* UJobComponent::SpawnProjectile(TSubclassOf<AProjectile> ProjectileClass, const FVector& Dir, float Speed, float MuzzleOffset, float MuzzleHeight)
{
	//자기 자신이 없거나 투사체 클래스가 없다면
	if (!OwnerCharacter || !ProjectileClass)
	{
		return nullptr;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector SpawnLocation =
		OwnerCharacter->GetActorLocation() + Dir * MuzzleOffset + FVector(0, 0, MuzzleHeight);
	const FRotator SpawnRotation = Dir.Rotation();

	// 풀에서 획득(재사용 또는 최초 1회만 스폰) — Destroy 대신 풀로 반환되는 수명 구조.
	AProjectile* Projectile = nullptr;
	if (UProjectilePoolSubsystem* Pool = World->GetSubsystem<UProjectilePoolSubsystem>())
	{
		Projectile = Pool->Acquire(ProjectileClass, SpawnLocation, SpawnRotation, OwnerCharacter, OwnerCharacter);
	}

	if (Projectile)
	{
		Projectile->Damage = GetDamage(); // 직업의 데미지(설계값+강화분)를 발사체에 전달
		Projectile->bDrawDebug = bDebugAttack; // 공격 디버그가 켜져 있으면 발사체 경로/적중범위도 그린다

		Projectile->MaxRange = EngageRange + ProjectileRangeMargin;
		Projectile->SetInitialSpeed(Speed);
	}
	return Projectile;
}


void UJobComponent::ApplyLevel(int32 Level)
{
	BonusLevelDamage = FMath::RoundToInt(Stats.Damage * DamageGrowth * (Level - 1));
	// hp
	if (OwnerCharacter)
	{
		OwnerCharacter->SetMaxHP(Stats.MaxHP * (1.0f + HPGrowth * (Level - 1)));
	}
}




// 스킬 레벨 가져와서 넘지 않으면 true
bool UJobComponent::IsSkillUnlocked() const
{
	if (!SkillData)
	{
		return false;
	}

	const int32 Level = OwnerCharacter ? OwnerCharacter->GetLevel() : 1;

	return Level >= SkillData->UnlockLevel;
}

// 쿨타임 관련
bool UJobComponent::CanUseSkill() const
{
	if (!OwnerCharacter || OwnerCharacter->GetIsDead() || bSkillActive || !IsSkillUnlocked())
	{
		return false;
	}

	// 생각해보아요 : 아니 이거 스킬이 여러 개면 안된다.
	return GetWorld()->GetTimeSeconds() >= SkillReadyTime;
}

void UJobComponent::StartSkillCooldown()
{
	const float Cooldown = SkillData ? SkillData->Cooldown : 0.0f;

	// 생각해보아요 이것도 스킬이 여러개면 안될듯한데. => 확장성 논란
	SkillReadyTime = GetWorld()->GetTimeSeconds() + Cooldown;
}

void UJobComponent::SetOwnerMovementLocked(bool bLocked)
{
	if (!OwnerCharacter)
	{
		return;
	}

	UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (bLocked)
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	else if (!OwnerCharacter->GetIsDead())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
}

// UI 표시용
float UJobComponent::GetSkillCooldownRatio() const
{
	if (bSkillActive)
	{
		return 1.0f;
	}

	const float Cooldown = SkillData ? SkillData->Cooldown : 0.0f;
	UWorld* World = GetWorld();

	if (Cooldown <= 0.0f || !World)
	{
		return 0.0f;
	}
	const float Remaining = SkillReadyTime - World->GetTimeSeconds();
	return FMath::Clamp(Remaining / Cooldown, 0.0f, 1.0f);
}

// 적을 찾아보자
TArray<AEnemy*> UJobComponent::FindEnemiesInRadius(const FVector& Center, float Radius) const
{
	TArray<AEnemy*> Out;
	if (!OwnerCharacter)
	{
		return Out;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
	//FCollisionQueryParams QueryParams;
	//QueryParams.AddIgnoredActor()

	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius));

	for (const FOverlapResult& Result : Overlaps)
	{
		AEnemy* Enemy = Cast<AEnemy>(Result.GetActor());
		if (Enemy && !Enemy->GetIsDead() && !Enemy->IsHidden())
		{
			Out.AddUnique(Enemy);
		}
	}
	return Out;
}
