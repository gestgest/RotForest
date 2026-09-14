// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/JobComponent.h"
#include "Characters/MyPlayer.h"
#include "Characters/CombatCharacter.h"
#include "Projectiles/Projectile.h"
#include "Projectiles/ProjectilePoolSubsystem.h"
#include "Engine/World.h"
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

// 무기 데이터는 캐릭터가 소유하므로 소유자가 없으면(CDO 등) 무기분은 빠진다.
int32 UJobComponent::GetDamage() const
{
	const int32 WeaponPower = OwnerCharacter ? OwnerCharacter->GetEquippedWeapon().WeaponPower : 0;
	return Stats.Damage + BonusDamage + WeaponPower;
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
			1.0f
		);
	}
}



/**
 * 원거리 용도
 * 전방으로 발사체를 스폰하고 직업의 Damage/속도를 적용해 반환한다 (궁수/마법사 공용).
 * @param ProjectileClass  스폰할 발사체 클래스
 * @param Speed            발사 속도(cm/s)
 * @param MuzzleOffset     캐릭터 앞쪽으로 스폰하는 거리(cm)
 * @param MuzzleHeight     스폰 높이 보정(cm)
 * @return 스폰된 발사체(실패 시 nullptr). 호출 측에서 폭발반경 등 추가 설정 가능.
 */

AProjectile* UJobComponent::SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight)
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

	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector SpawnLocation =
		OwnerCharacter->GetActorLocation() + Forward * MuzzleOffset + FVector(0, 0, MuzzleHeight);
	const FRotator SpawnRotation = Forward.Rotation();

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
		Projectile->SetInitialSpeed(Speed);
	}
	return Projectile;
}

