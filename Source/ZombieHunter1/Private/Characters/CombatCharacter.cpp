// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CombatCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h" //머리 위 HP 바
#include "UI/EnemyHPBarWidget.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 공격 몽타주 Notify → HandleAttackNotify (서브클래스가 실제 공격을 처리).
	// 플레이어/동료/적이 똑같이 쓰던 배선을 여기로 모았다.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &ACombatCharacter::OnMontageNotifyBegin);
		}
	}
}

void ACombatCharacter::AddHP(int32 add_hp)
{
	SetHP(HP + add_hp);
}

void ACombatCharacter::SetHP(int32 new_hp)
{
	HP = new_hp;
	SetDead(HP <= 0);
	UpdateHPBar(); // 머리 위 바가 있는 캐릭터(적/동료)만 실제로 갱신된다
}

// 머리 위 HP 바 생성 — 반드시 서브클래스 생성자에서 호출(CreateDefaultSubobject 제약).
// 스크린 스페이스라 탑다운 카메라에서도 항상 화면을 향하고 크기가 일정하다.
void ACombatCharacter::CreateHPBarComponent()
{
	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBar"));
	HPBarComponent->SetupAttachment(RootComponent);
	HPBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // 캡슐(반높이 ~88) 위
	HPBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarComponent->SetDrawSize(FVector2D(100.0f, 12.0f));
	HPBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACombatCharacter::UpdateHPBar()
{
	if (!HPBarComponent)
	{
		return; // 플레이어 등 바 없는 캐릭터
	}

	if (UEnemyHPBarWidget* Bar = Cast<UEnemyHPBarWidget>(HPBarComponent->GetWidget()))
	{
		Bar->SetHPPercent(MaxHP > 0 ? (float)HP / (float)MaxHP : 0.0f);
	}

	// 죽으면 시체 위에 바가 남지 않게 숨긴다. 풀 재사용/부활(SetHP>0) 시 다시 보인다.
	HPBarComponent->SetVisibility(!IsDead);
}

void ACombatCharacter::SetDead(bool bNewDead)
{
	const bool bWasDead = IsDead;
	IsDead = bNewDead;

	// 전환 시점에만 1회씩 — 죽음/부활 처리가 중복 발동하지 않게.
	if (bNewDead && !bWasDead)
	{
		OnDeath();
	}
	else if (!bNewDead && bWasDead)
	{
		OnRevive();
	}
}

UAnimMontage* ACombatCharacter::GetAttackMontageForJob(EJobType JobType) const
{
	// 이 캐릭터의 스켈레톤에 맞는 직업별 몽타주가 있으면 그걸, 없으면 단일 AttackMontage로 폴백.
	if (UAnimMontage* const* Found = JobAttackMontages.Find(JobType))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	return AttackMontage;
}

void ACombatCharacter::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	HandleAttackNotify(NotifyName);
}

// 실제 타격 판정/사운드는 현재 직업이 담당한다 (전사: 근접 스윕, 궁수: 발사체 등).
// 직업이 없는 캐릭터(AEnemy)는 이 함수를 재정의해 자기 방식으로 때린다.
void ACombatCharacter::HandleAttackNotify(FName NotifyName)
{
	if (CurrentJob)
	{
		CurrentJob->OnAttackNotify(NotifyName);
	}
}

// 직업 컴포넌트 생성 — 플레이어/동료가 똑같이 쓰던 코드를 여기로 모았다.
// 스탯 적용(MaxHP/이동속도)은 InitializeForOwner 안에서 처리한다.
void ACombatCharacter::CreateJobComponent()
{
	if (!DefaultJobClass)
	{
		return; // 직업 없는 캐릭터(적 등)
	}

	// 이미 직업이 있으면 떼어낸다 → 직업 교체에도 이 함수를 그대로 쓸 수 있다.
	if (CurrentJob)
	{
		CurrentJob->DestroyComponent();
		CurrentJob = nullptr;
	}

	CurrentJob = NewObject<UJobComponent>(this, DefaultJobClass);
	if (CurrentJob)
	{
		CurrentJob->RegisterComponent();
		CurrentJob->InitializeForOwner(this);
	}
}

float ACombatCharacter::GetAttackInterval() const
{
	// 직업이 간격을 정한다. 직업이 없거나 값이 안 잡혀 있으면 캐릭터의 폴백을 쓴다.
	// (폴백이 없으면 간격 0 → 매 프레임 공격이 되므로 반드시 0보다 큰 값을 보장한다)
	if (CurrentJob && CurrentJob->Stats.AttackInterval > 0.0f)
	{
		return CurrentJob->Stats.AttackInterval;
	}
	return AttackInterval > 0.0f ? AttackInterval : 0.4f;
}

bool ACombatCharacter::TickAttack(float DeltaTime, bool bWantsToAttack)
{
	TimeSinceLastAttack += DeltaTime;

	if (!bWantsToAttack || !CurrentJob || IsDead)
	{
		return false;
	}
	if (TimeSinceLastAttack < GetAttackInterval())
	{
		return false;
	}

	TimeSinceLastAttack = 0.0f;
	CurrentJob->Attack(); // 직업이 공격 방식을 결정(몽타주 재생 → Notify → OnAttackNotify)
	return true;
}
