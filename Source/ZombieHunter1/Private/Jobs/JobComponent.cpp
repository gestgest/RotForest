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

	// 직업 기본 무기를 실제로 장착시킨다. 이 뒤로 "지금 무기"는 EquippedWeapon 하나뿐이다.
	// EquipWeapon()을 쓰지 않는 이유: DefaultWeapon의 JobType이 BP에서 안 맞춰져 있으면
	// 자기 기본 무기가 직업 제한에 걸려 거부당한다.
	EquippedWeapon = DefaultWeapon;
	RefreshWeaponMesh();
}

// 장착 무기의 메시를 소유자의 무기 슬롯에 끼운다.
// 어느 손에 끼울지도 직업이 정한다 — 궁수는 왼손, 나머지는 오른손.
void UJobComponent::RefreshWeaponMesh()
{
	if (!OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->EquipWeaponInHand(EquippedWeapon.Mesh, WeaponHand);
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

// 무기 획득처(드랍/상점/발판)는 전부 이 함수 하나로 들어온다.
bool UJobComponent::EquipWeapon(const FWeaponItemData& Item)
{
	// 다른 직업 전용 무기는 거부한다 — 검사가 활을 들면 몽타주도 공격 판정도 전부 어긋난다.
	if (Item.JobType != JobType)
	{
		return false;
	}

	EquippedWeapon = Item;   // 값 복사 — 원본(픽업 액터/배열 원소)이 사라져도 안전하다.
	RefreshWeaponMesh();     // 데이터가 바뀌었으니 손에 든 것도 갱신한다.
	return true;
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

