// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CombatCharacter.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h" //머리 위 HP 바
#include "UI/EnemyHPBarWidget.h"
#include "Characters/CombatRegistrySubsystem.h"

#include "Engine/Engine.h" //화면 디버그 메시지(AddOnScreenDebugMessage)

DEFINE_LOG_CATEGORY(LogWeapon);

namespace
{
	// 무기 슬롯 상태를 화면 좌상단에 색으로 띄운다.
	// 출력 로그는 verbosity로만 색이 정해지지만(Error=빨강/Warning=노랑), 화면 메시지는 색을 마음대로 줄 수 있다.
	// 캐릭터가 여럿일 때 어느 BP 문제인지 바로 알 수 있게 액터 이름을 앞에 붙인다.
	void WeaponScreenMsg(const AActor* Owner, const FColor& Color, const FString& Msg)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, Color, FString::Printf(TEXT("[%s] %s"), *GetNameSafe(Owner), *Msg));
		}
	}
}

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 무기 슬롯은 양손 고정 2개. C++이 소유하므로 BP에서 추가할 필요가 없고,
	// BP가 부모 참조를 잃어도 슬롯이 사라지지 않는다.
	// 부착 소켓은 여기서 한 번 잡고, BP에서 소켓 이름을 바꿨다면 BeginPlay에서 다시 붙인다.
	WeaponRight = CreateDefaultSubobject<UChildActorComponent>(TEXT("WeaponRight"));
	WeaponRight->SetupAttachment(GetMesh(), RightHandSocket);

	WeaponLeft = CreateDefaultSubobject<UChildActorComponent>(TEXT("WeaponLeft"));
	WeaponLeft->SetupAttachment(GetMesh(), LeftHandSocket);
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCombatRegistrySubsystem* Reg = GetWorld()->GetSubsystem<UCombatRegistrySubsystem>())
	{
		Reg->Register(this);
	}
	InitWeaponSlot();

	// 공격 몽타주 Notify → HandleAttackNotify (서브클래스가 실제 공격을 처리).
	// 플레이어/동료/적이 똑같이 쓰던 배선을 여기로 모았다.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &ACombatCharacter::OnMontageNotifyBegin);
			Anim->OnMontageEnded.AddDynamic(this, &ACombatCharacter::OnAttackMontageEnded);
		}
	}
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//제거
	if (UCombatRegistrySubsystem* Reg = GetWorld()->GetSubsystem<UCombatRegistrySubsystem>())
	{
		Reg->Unregister(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ACombatCharacter::ApplyJobStats(FJobStats Stats)
{
	MaxHP = Stats.MaxHP;
	SetHP(Stats.MaxHP);
	Damage = Stats.Damage;
}


// 양손 무기 슬롯을 실제로 세운다 — 소켓 부착 → Weapon_BP 스폰 → 안쪽 메시 캐시.
// 손이 바뀌는 건 '어느 슬롯을 켜느냐'로 처리되므로, 여기서는 둘 다 준비만 하고 숨겨둔다.
void ACombatCharacter::InitWeaponSlot()
{
	WeaponMeshComponent = nullptr;
	WeaponRightMesh = nullptr;
	WeaponLeftMesh = nullptr;

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// WeaponActorClass를 비워둔 건 "이 캐릭터는 무기 슬롯을 안 쓴다"는 선언이다(적 등).
	// 잘못된 설정이 아니므로 소켓 검사도, 경고도 하지 않고 조용히 빠진다.
	if (!WeaponActorClass)
	{
		UE_LOG(LogWeapon, Verbose, TEXT("[%s] 무기 슬롯 미사용(WeaponActorClass 없음)."), *GetName());
		return;
	}

	// 슬롯 하나를 세우는 절차는 좌우가 완전히 같다 — 소켓 이름만 다르다.
	// 실패 지점(소켓/클래스/메시)마다 무엇이 잘못됐고 어디를 고쳐야 하는지까지 로그로 남긴다.
	auto SetupSlot = [&](UChildActorComponent* Slot, FName SocketName, const TCHAR* HandLabel) -> USkeletalMeshComponent*
	{
		if (!Slot)
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] %s 슬롯 컴포넌트 자체가 없다(C++ 생성 실패)."), *GetName(), HandLabel);
			WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("%s 슬롯 컴포넌트 없음"), HandLabel));
			return nullptr;
		}

		// 소켓 확인 — 없는 이름에 붙이면 에러 없이 캐릭터 원점(발밑)에 붙어버려서 원인 찾기가 고약하다.
		if (MeshComp->DoesSocketExist(SocketName))
		{
			// KeepRelativeTransform: 슬롯 컴포넌트에 설정해둔 Relative Location/Rotation을 유지한 채 붙인다.
			// 무기를 잡는 각도는 이 상대 회전으로 맞춘다 — 캐릭터 BP의 WeaponRight/WeaponLeft Details에서 조정.
			// (SnapToTarget을 쓰면 상대 트랜스폼이 리셋돼서 BP에서 맞춘 각도가 통째로 날아간다)
			Slot->AttachToComponent(MeshComp, FAttachmentTransformRules::KeepRelativeTransform, SocketName);
			UE_LOG(LogWeapon, Log, TEXT("[%s] %s: 소켓 '%s' 부착 OK"), *GetName(), HandLabel, *SocketName.ToString());
		}
		else
		{
			// 스켈레톤에 소켓도 본도 그 이름이 없다 → BP의 Right/Left Hand Socket 값 또는 스켈레톤 소켓을 고쳐야 함.
			UE_LOG(LogWeapon, Error, TEXT("[%s] %s: 소켓/본 '%s'이(가) '%s' 스켈레톤에 없다. BP의 %s Hand Socket 값을 확인할 것."),
				*GetName(), HandLabel, *SocketName.ToString(), *GetNameSafe(MeshComp->GetSkinnedAsset()),
				HandLabel);
			WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("%s: 소켓 '%s' 없음"), HandLabel, *SocketName.ToString()));
		}

		// 무기 액터(Weapon_BP)를 이 슬롯에 스폰. 양손이 같은 클래스를 쓴다(빈 껍데기이므로).
		// WeaponActorClass가 유효한 건 위에서 이미 보장됐다.
		if (Slot->GetChildActorClass() != WeaponActorClass)
		{
			Slot->SetChildActorClass(WeaponActorClass);
		}

		AActor* Child = Slot->GetChildActor();
		if (!Child)
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] %s: '%s' 액터 스폰 실패."), *GetName(), HandLabel, *GetNameSafe(WeaponActorClass));
			WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("%s: 무기 액터 스폰 실패"), HandLabel));
			return nullptr;
		}

		USkeletalMeshComponent* InnerMesh = Child->FindComponentByClass<USkeletalMeshComponent>();
		if (!InnerMesh)
		{
			// Weapon_BP 안에 StaticMesh만 있는 경우가 흔한 원인. 무기는 SkeletalMesh여야 한다(활 시위 애니 때문).
			UE_LOG(LogWeapon, Error, TEXT("[%s] %s: '%s' 안에 SkeletalMeshComponent가 없다."),
				*GetName(), HandLabel, *GetNameSafe(WeaponActorClass));
			WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("%s: 무기 BP에 SkeletalMesh 없음"), HandLabel));
			return nullptr;
		}

		InnerMesh->SetVisibility(false); // 직업이 장착할 때까지는 숨김
		UE_LOG(LogWeapon, Log, TEXT("[%s] %s: 슬롯 준비 완료"), *GetName(), HandLabel);
		if (bDebugCombat)
		{
			WeaponScreenMsg(this, FColor::Green, FString::Printf(TEXT("%s 슬롯 준비 완료 (%s)"), HandLabel, *SocketName.ToString()));
		}
		return InnerMesh;
	};

	WeaponRightMesh = SetupSlot(WeaponRight, RightHandSocket, TEXT("오른손"));
	WeaponLeftMesh = SetupSlot(WeaponLeft, LeftHandSocket, TEXT("왼손"));
}


// 무기 교체 — 어느 손에 끼울지는 직업이 넘겨준 Hand가 정한다.
// 캐릭터는 직업 종류를 모른다. 분기는 손 개수(2)로 고정이라 직업이 늘어도 여기는 안 늘어난다.
void ACombatCharacter::EquipWeaponInHand(USkeletalMesh* NewMesh, EWeaponHand Hand)
{
	const bool bLeft = (Hand == EWeaponHand::Left);
	USkeletalMeshComponent* TargetMesh = bLeft ? WeaponLeftMesh : WeaponRightMesh;
	USkeletalMeshComponent* OtherMesh = bLeft ? WeaponRightMesh : WeaponLeftMesh;

	const TCHAR* HandLabel = bLeft ? TEXT("왼손") : TEXT("오른손");

	if (!TargetMesh && OtherMesh)
	{
		// 쓰려는 손 슬롯이 없다(소켓이 없어 스폰 실패 등). 무기를 못 들게 하느니 반대 손에라도 들린다.
		UE_LOG(LogWeapon, Error, TEXT("[%s] %s 슬롯이 준비 안 돼 반대 손으로 대체한다. 위쪽 %s 슬롯 에러를 먼저 볼 것."),
			*GetName(), HandLabel, HandLabel);
		WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("%s 슬롯 없음 → 반대 손으로 장착"), HandLabel));
		Swap(TargetMesh, OtherMesh);
	}

	// 반대 손은 반드시 숨긴다.
	// 이걸 빼먹으면 직업을 바꿨을 때 이전 직업 무기가 반대 손에 그대로 남는다(왼손 활 + 오른손 검).
	if (OtherMesh)
	{
		OtherMesh->SetVisibility(false);
	}

	// 현재 장착 슬롯을 기억해둔다 — 궁수의 활 시위 애니처럼 무기 메시를 직접 만지는 직업이 이걸 집어간다.
	WeaponMeshComponent = TargetMesh;
	if (!TargetMesh)
	{
		// 슬롯이 아예 없는 캐릭터에 무기를 든 직업이 붙은 경우 — 이건 진짜 설정 실수다.
		// (무기 없는 직업이면 NewMesh도 null이므로 조용히 넘어간다)
		if (NewMesh)
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] 무기 '%s'을(를) 줄 슬롯이 없다. 캐릭터 BP에 Weapon Actor Class를 지정할 것."),
				*GetName(), *GetNameSafe(NewMesh));
			WeaponScreenMsg(this, FColor::Red, TEXT("무기 슬롯 없음 — Weapon Actor Class 미지정"));
		}
		return;
	}

	// 슬롯의 메시만 교체. 컴포넌트 자체는 유지되므로 부착 소켓도 그대로다.
	TargetMesh->SetSkeletalMeshAsset(NewMesh);
	// 무기가 없는 직업(NewMesh == null)은 숨긴다.
	TargetMesh->SetVisibility(NewMesh != nullptr);

	UE_LOG(LogWeapon, Log, TEXT("[%s] %s에 무기 '%s' 장착"), *GetName(), HandLabel, *GetNameSafe(NewMesh));
	if (bDebugCombat)
	{
		WeaponScreenMsg(this, FColor::Green, FString::Printf(TEXT("%s에 '%s' 장착"), HandLabel, *GetNameSafe(NewMesh)));
	}
}

// 오른손 단축형
void ACombatCharacter::EquipWeapon(USkeletalMesh* NewMesh)
{
	EquipWeaponInHand(NewMesh, EWeaponHand::Right);
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
	return DefaultAttackMontage;
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



// 공격 몽타주가 끝나는 순간 = 활시위를 놓는 순간(애니 마지막 프레임)인 직업용.
// 전사처럼 중간에 타격하는 직업은 Notify를 쓰므로 여기 반응하지 않는다.
void ACombatCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		return; // 취소된 공격은 발사하지 않는다
	}
	if (CurrentJob)
	{
		CurrentJob->OnAttackMontageEnded();
	}
}
