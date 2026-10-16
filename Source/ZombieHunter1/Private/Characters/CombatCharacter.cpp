// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CombatCharacter.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h" //머리 위 HP 바
#include "UI/EnemyHPBarWidget.h"
#include "Characters/CombatRegistrySubsystem.h"
#include "Items/ItemDataAsset.h"

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

	WeaponSlot = CreateDefaultSubobject<UChildActorComponent>(TEXT("WeaponSlot"));
	WeaponSlot->SetupAttachment(GetMesh(), DefaultWeaponSocket);
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


// 손이 바뀌는 건 '어느 슬롯을 켜느냐'로 처리되므로, 여기서는 둘 다 준비만 하고 숨겨둔다.
void ACombatCharacter::InitWeaponSlot()
{
	WeaponMeshComponent = nullptr;

	if (!GetMesh() || !WeaponSlot)
	{
		return;
	}

	// WeaponActorClass를 비워둔 건 "이 캐릭터는 무기 슬롯을 안 쓴다"는 선언이다(적 등).
	if (!WeaponActorClass)
	{
		UE_LOG(LogWeapon, Verbose, TEXT("[%s] 무기 슬롯 미사용(WeaponActorClass 없음)."), *GetName());
		return;
	}


	if (WeaponSlot->GetChildActorClass() != WeaponActorClass)
	{
		WeaponSlot->SetChildActorClass(WeaponActorClass);
	}

	AActor* Child = WeaponSlot->GetChildActor();

	if (!Child)
	{
		UE_LOG(LogWeapon, Error, TEXT("[%s] '%s' 액터 스폰 실패"), *GetName(), *GetNameSafe(WeaponActorClass));
		WeaponScreenMsg(this, FColor::Red, TEXT("무기 액터 스폰 실패"));
		return;
	}

	// 무기안에 SKM 찾기
	WeaponMeshComponent = Child->FindComponentByClass<USkeletalMeshComponent>();
	if (!WeaponMeshComponent)
	{
		UE_LOG(LogWeapon, Error, TEXT("[%s] %s에 스켈레톤 매시 없음"), *GetName(), *GetNameSafe(WeaponActorClass));
		WeaponScreenMsg(this, FColor::Red, TEXT("스켈레톤 매시 없음"));
		return;
	}

	// 장착할때까지 숨김
	WeaponMeshComponent->SetVisibility(false);
	UE_LOG(LogWeapon, Log, TEXT("[%s] 장착 준비 완료"), *GetName());
}


bool ACombatCharacter::AttachWeaponSlot(FName Socket)
{
	USkeletalMeshComponent* MeshComp = GetMesh();

	if (!MeshComp || !WeaponSlot)
	{
		return false;
	}

	// 없는 소켓에 붙이면 발밑으로 (Error)
	if (!MeshComp->DoesSocketExist(Socket))
	{
		UE_LOG(LogWeapon, Error, TEXT("[%s] 소켓 '%s'이(가) '%s' 스켈레톤에 없다. 무기 DA의 Socket Name을 확인할 것."),
			*GetName(), *Socket.ToString(), *GetNameSafe(MeshComp->GetSkinnedAsset()));
		WeaponScreenMsg(this, FColor::Red, FString::Printf(TEXT("소켓 '%s' 없음"), *Socket.ToString()));
		return false;
	}

	// 핵심 장착 (소켓에 MeshComps)
	WeaponSlot->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);

	return true;
	
}



// 무기 획득처(픽업/상점/보상)는 전부 이 함수 하나로 들어온다.
bool ACombatCharacter::EquipWeaponItem(UWeaponDataAsset* Item)
{
	if (!Item || !CurrentJob)
	{
		return false;
	}

	//잡 타입이 다르면
	if (Item->JobType != CurrentJob->JobType)
	{
		return false;
	}
	EquippedWeapon = Item;   // 에셋 참조 — 픽업 액터가 사라져도 정의는 남는다.
	RefreshWeaponMesh();     // 데이터가 바뀌었으니 손에 든 것도 갱신한다.
	return true;
}

// 장착은 하지 않고 판단만 한다 — 후보 중에서 고르려면 둘이 분리돼 있어야 한다.
bool ACombatCharacter::WantsWeaponItem(UWeaponDataAsset* Item) const
{
	if (!Item || !CurrentJob)
	{
		return false;
	}

	//잡 타입이 다르면
	if (Item->JobType != CurrentJob->JobType)
	{
		return false;
	}

	return Item->WeaponPower > GetEquippedWeaponPower();
}

int32 ACombatCharacter::GetEquippedWeaponPower() const
{
	return EquippedWeapon ? EquippedWeapon->WeaponPower : 0;
}


// DA를 참고해서 메시를 끼움
void ACombatCharacter::RefreshWeaponMesh()
{
	USkeletalMesh* NewMesh = EquippedWeapon ? EquippedWeapon->Mesh : nullptr;
	
	if (!WeaponMeshComponent)
	{
		if (NewMesh)
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] 무기 '%s'를 줄 슬롯이 없다. 캐릭터 BP에 Weapon Class를 지정할 것"), *GetName(), *GetNameSafe(NewMesh));
			WeaponScreenMsg(this, FColor::Red, TEXT("무기 슬롯 없음 - Weapon Actor Class 미지정"));
		}
		return;
	}

	FName Socket;
	if (!EquippedWeapon || EquippedWeapon->SocketName.IsNone())
	{
		Socket = DefaultWeaponSocket;
	}
	else
	{
		Socket = EquippedWeapon->SocketName;
	}
	AttachWeaponSlot(Socket);

	//메시
	WeaponMeshComponent->SetSkeletalMeshAsset(NewMesh);
	WeaponMeshComponent->SetVisibility(NewMesh != nullptr);

	//쥐는 각도
	if (EquippedWeapon)
	{
		WeaponMeshComponent->SetRelativeLocationAndRotation(EquippedWeapon->GripLocation, EquippedWeapon->GripRotation);
	}

	//FString 단위
	UE_LOG(LogWeapon, Log, TEXT("[%s] 무기 '%s' 장착 (소켓 '%s')"), *GetName(), *GetNameSafe(NewMesh), *Socket.ToString());
	if (bDebugCombat)
	{
		WeaponScreenMsg(this, FColor::Green, FString::Printf(TEXT("'%s' 장착"), *GetNameSafe(NewMesh)));
	}
}


void ACombatCharacter::AddHP(int32 add_hp)
{
	SetHP(HP + add_hp);
}

void ACombatCharacter::SetHP(int32 new_hp)
{
	const int32 OldHP = HP;
	HP = new_hp;
	SetDead(HP <= 0);
	UpdateHPBar(); // 머리 위 바가 있는 캐릭터(적/동료)만 실제로 갱신된다
	
	// 오버킬 방지 : HP30인데 데미지가 50이면 음수임
	const int32 Delta = FMath::Max(HP, 0) - FMath::Max(OldHP, 0);
	if (Delta != 0) //피해를 입힌경우
	{
		OnHPChanged.Broadcast(HP, Delta);
	}
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

	// 아군: 살아있으면 항상 / 적: 맞았을 때만
	const bool bDamaged = HP < MaxHP;
	const bool bShow = !IsDead && (TeamType == ETeam::Ally || bDamaged);

	// 죽으면 시체 위에 바가 남지 않게 숨긴다. 풀 재사용/부활(SetHP>0) 시 다시 보인다.
	HPBarComponent->SetVisibility(bShow);
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

//DesiredAimPoint 설정
void ACombatCharacter::SetDesiredAimPoint(const FVector& Point)
{
	DesiredAimPoint = Point;
	bHasDesiredAimPoint = true;
}

void ACombatCharacter::ClearDesiredAimPoint()
{
	bHasDesiredAimPoint = false;
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

//공격
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

	// 공격 방향은 지금 고정한다 — 타격은 몽타주 Notify라 0.2~0.4초 뒤에 일어난다.
	AttackAimDir = GetActorForwardVector();

	//최종적으로 point는 이때
	AttackAimPoint = DesiredAimPoint;
	bHasAttackAimPoint = bHasDesiredAimPoint;

	CurrentJob->Attack(); // 직업이 공격 방식을 결정(몽타주 재생 → Notify → OnAttackNotify)
	return true;
}


// 공식 공격 방향 반환
FVector ACombatCharacter::GetAttackAimDir() const
{
    // 데스크탑 코드
	if (bHasAttackAimPoint)
	{
		//Attack 방향
		FVector AttackVector = AttackAimPoint - GetActorLocation();
		AttackVector.Z = 0;

		//normal
		AttackVector = AttackVector.GetSafeNormal();
		if (!AttackVector.IsNearlyZero())
		{
			return AttackVector;
		}
	}

    // 마우스는 여기에 추가될 예정
    // 대체로 모바일은 AttackAimDir를 반환한다.
	return AttackAimDir.IsNearlyZero() ? GetActorForwardVector() : AttackAimDir;
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
