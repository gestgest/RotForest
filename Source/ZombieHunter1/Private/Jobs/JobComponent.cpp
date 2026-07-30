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

	EquipWeapon(); // 직업 무기를 소유자 무기 슬롯에 끼운다
}

void UJobComponent::EquipWeapon()
{
	// 무기 메시 교체는 플레이어의 무기 슬롯(ChildActor) 시스템 전용이다.
	// 동료(ACompanion)는 자기 BP 메시를 그대로 쓰므로 여기서 건드리지 않는다.
	// WeaponMesh가 null이면 무기가 숨겨진다(예: 지팡이 없는 마법사).
	if (AMyPlayer* Player = Cast<AMyPlayer>(OwnerCharacter))
	{
		Player->SetWeaponMesh(WeaponMesh);
	}
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

//원거리 용도
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
