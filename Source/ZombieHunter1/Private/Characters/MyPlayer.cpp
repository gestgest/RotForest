// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/MyPlayer.h"
#include "Characters/Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h" //BindAxisKey
#include "InputCoreTypes.h"            //EKeys
#include "Kismet/GameplayStatics.h" //getCharacter, sound
#include "Animation/AnimInstance.h" //SetRootMotionMode, ERootMotionMode
#include "Animation/AnimMontage.h" //공격 몽타주 길이


#include "GameFramework/GameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h" //GetCharacterMovement
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "NavigationInvokerComponent.h"
#include "UI/VirtualJoystick.h"
#include "Engine/Engine.h" //GEngine 화면 디버그
#include "Jobs/JobComponent.h"
#include "Jobs/WarriorJob.h"
#include "Components/SlateWrapperTypes.h" //ESlateVisibility
#include "Components/SkeletalMeshComponent.h" //무기 컴포넌트 메시 교체
#include "Components/ChildActorComponent.h" //무기 ChildActor(Weapon_BP)
#include "Engine/SkeletalMesh.h"
#include "Characters/PartyComponent.h" //동료 파티(섭외·장비 배분)
#include "ZombieGameInstance.h" //직업 선택 씬에서 고른 직업 읽기
#include "ZombieSlayerGameMode.h" //사망 시 적 시간 정지(SetEnemiesFrozen)


// 모바일(안드로이드/iOS) 플랫폼이면 true. 터치 조이스틱 표시 여부 판단용.
// 컴파일 타임 매크로라 PC 빌드에선 항상 false → 조이스틱 숨김.
static bool IsMobilePlatform()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    return true;
#else
    return false;
#endif
}


// 생성자
AMyPlayer::AMyPlayer()
{
 	// Tick() 업데이트 키는 변수
	PrimaryActorTick.bCanEverTick = true;

    //디폴트 설정
    TeamType = ETeam::Ally;

    InitController();
    InitCamera();

	// NavMesh 동적 생성 : 플레이어 주변에만 NavMesh를 깔고, 멀어지면 제거.
	// 무한 청크 맵의 시야 범위(약 ViewRadius*ChunkSize)를 덮도록 반경 설정.
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(7000.0f, 9000.0f);

	// 동료 파티 — 섭외/명단/장비 배분은 전부 이쪽이 맡는다.
	Party = CreateDefaultSubobject<UPartyComponent>(TEXT("Party"));

	// 기본 직업: 전사. 에디터(BP)에서 DefaultJobClass를 바꾸면 다른 직업으로 시작한다.
	DefaultJobClass = UWarriorJob::StaticClass();
}


void AMyPlayer::BeginPlay()
{
	Super::BeginPlay();

    OnTopDownMode();

    //플레이어 스타팅 찾기
    if (AGameModeBase* gameMode = GetWorld()->GetAuthGameMode())
    {
         playerStart = gameMode->FindPlayerStart(PlayerControllerRef);
    }


    UAnimInstance* animInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());

    if (animInstance)
    {
        // 공격 Notify 배선은 베이스(ACombatCharacter)가 처리한다(→ HandleAttackNotify).

        // 공격 몽타주의 루트 모션이 캐릭터 이동을 덮어써서 공격 중 못 움직이는 문제 해결.
        // IgnoreRootMotion: 루트 모션을 추출해 메시는 제자리에 고정하되 이동에는 적용하지 않음.
        // → 공격 중에도 AddMovementInput(이동 입력)이 그대로 캐릭터를 움직임.
        animInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    }


    ReStart(); // 주의: 엔진 내장 APawn::Restart()가 아니라 우리 부활 함수(대문자 S). HP/돈 초기화 + 스타트 지점 이동.

    // 직업(Job) 컴포넌트 생성 — 시작 시 1개 고정.
    if (!DefaultJobClass)
    {
        DefaultJobClass = UWarriorJob::StaticClass();
    }

    SetupJob();
}

void AMyPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 죽으면 입력 처리를 멈춤 (죽은 뒤 공격 몽타주가 또 재생되는 것 방지)
    if (IsDead)
    {
        return;
    }

    // 마우스를 기존 스틱 포맷으로 변환: just like 로스트아크
    // 커서 방향 (dx,dy) → FVector2D(Y=dx, X=dy).
    FVector2D MouseMove = FVector2D::ZeroVector;
    FVector2D MouseAim = FVector2D::ZeroVector;

    MouseInput(MouseMove, MouseAim);

    // 게임패드 / 터치 / 마우스 중 가장 크게 입력된 쪽을 사용 (전부 지원) => 람다임
    auto Largest = [](const FVector2D& A, const FVector2D& B, const FVector2D& C) -> FVector2D
        {
            const FVector2D& AB = (A.SizeSquared() >= B.SizeSquared()) ? A : B;
            return (AB.SizeSquared() >= C.SizeSquared()) ? AB : C;
        };
    const FVector2D Move = Largest(TouchMove, GamepadMove, MouseMove);
    const FVector2D Aim = Largest(TouchAim, GamepadAim, MouseAim);

    DebugWalkSpeed(Move);

    UpdateMovement(DeltaTime, Move);
    UpdateAimAndAttack(DeltaTime, Aim, Move);

    // 다리를 이동 방향으로 돌리기 위한 각도 갱신(상체/조준은 액터 회전 그대로). AnimBP가 LegYawOffset을 읽는다.
    UpdateLegYawOffset(DeltaTime);

    // 직업 패시브(힐러 자가 회복 등) — 살아있을 때만 (위에서 HP<=0이면 return)
    if (CurrentJob)
    {
        CurrentJob->TickJob(DeltaTime);
    }
}



// [생성자 함수들]
void AMyPlayer::InitController()
{
    // 컨트롤러 회전이 캐릭터를 돌리지 않게 함 (조준 방향으로 직접 회전시킴)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;


    //// 이동 방향 자동 회전 끄기 — 오른쪽 스틱(조준) 방향으로 수동 회전
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AMyPlayer::InitCamera()
{
    // 비스듬한 탑다운 카메라 암
    TopDownBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("TopDownBoom"));
    TopDownBoom->SetupAttachment(RootComponent);
    TopDownBoom->SetUsingAbsoluteRotation(true); // 캐릭터가 회전해도 카메라는 고정
    TopDownBoom->TargetArmLength = CameraDistance;
    TopDownBoom->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
    TopDownBoom->bDoCollisionTest = false; // 탑다운: 벽에 의해 줌인되지 않게

    // 탑다운 카메라
    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(TopDownBoom, USpringArmComponent::SocketName);
    TopDownCamera->bUsePawnControlRotation = false;
}




void AMyPlayer::OnTopDownMode()
{
    //카메라
    // 에디터에서 수정한 카메라 값 적용
    if (TopDownBoom)
    {
        TopDownBoom->TargetArmLength = CameraDistance;
        TopDownBoom->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
    }

    // BP에 남아있는 옛 카메라(3인칭 등)를 끄고, 탑다운 카메라만 활성화
    // → 블루프린트를 안 건드려도 탑다운 시점이 화면 카메라로 잡힘
    if (TopDownCamera)
    {
        TArray<UCameraComponent*> Cameras;
        GetComponents<UCameraComponent>(Cameras);
        for (UCameraComponent* Cam : Cameras)
        {
            Cam->SetActive(Cam == TopDownCamera);
        }

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            PC->SetViewTargetWithBlend(this, 0.0f);
        }
    }



    PlayerControllerRef = Cast<APlayerController>(GetController());

    // 탑다운: 컨트롤러 yaw를 0으로 고정하고 마우스 Look 입력을 무시한다.
    // → BP의 "컨트롤 회전 기준 이동"이 월드축(+X/+Y) 고정 이동과 동일해지고,
    //   마우스 때문에 직진이 휘는 yaw 드리프트도 사라진다. (카메라는 절대회전이라 무관)
    if (PlayerControllerRef)
    {
        PlayerControllerRef->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
        PlayerControllerRef->SetIgnoreLookInput(true);

        // 마우스 커서 표시 + 게임/UI 동시 입력 (데스크탑에서 마우스로 조이스틱 조작)
        PlayerControllerRef->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PlayerControllerRef->SetInputMode(InputMode);
    }
}


void AMyPlayer::SetupJob()
{
    // 직업 선택 씬에서 고른 직업이 GameInstance에 실려 왔으면 그걸 우선 사용한다.
    // 선택이 없거나(None) GameInstance 클래스를 아직 지정 안 했으면(캐스트 실패)
    // 기존 DefaultJobClass 그대로 — 에디터 셋업 전에도 동작이 바뀌지 않는다.

    if (UZombieGameInstance* GI = Cast<UZombieGameInstance>(GetGameInstance()))
    {
        if (GI->SelectedJobClass)
        {
            DefaultJobClass = GI->SelectedJobClass;
        }
    }

    // 직업 컴포넌트 생성/부착은 베이스(ACombatCharacter)가 담당한다 — 동료와 동일한 경로.
    CreateJobComponent();

    if (CurrentJob)
    {
        // 공격 몽타주는 캐릭터(JobAttackMontages[JobName])가 소유 — 직업으로 승계하지 않는다.
        // 사운드만 직업이 비어 있을 때 플레이어 것을 물려준다.
        if (!CurrentJob->GetAttackSound())
        {
            CurrentJob->SetAttackSound(AttackSound);
        }
    }
}









//마우스의 이동, 공격을 담당
void AMyPlayer::MouseInput(FVector2D & MouseMove, FVector2D & MouseAim)
{
    if (!bRightMouseHeld && !bLeftMouseHeld)
        return;

    // 이번 프레임의 커서 방향을 구한다.
    FVector MoveDir = FVector::ZeroVector;
    FVector Cursor;

    //커서 지면에 닿았는지 여부 => Cursor에 클릭한 값 넣기
    if (GetCursorGroundLocation(Cursor)) 
    {
        LastCursorPoint = Cursor;
        bHasLastCursorPoint = true;

        FVector ToCursor = Cursor - GetActorLocation();
        ToCursor.Z = 0.0f;

        // [이동]
        // CursorStopRadius는 너무 가까우면 player가 진자운동하는 버그 해결용
        if (ToCursor.SizeSquared() > CursorStopRadius * CursorStopRadius && ToCursor.Normalize())
        {
            LastCursorDir = ToCursor; // 유효 방향 캐시
            MoveDir = ToCursor;
        }


        // 마우스 공격 에임 설정
        if (!MoveDir.IsNearlyZero() && bLeftMouseHeld)
        {
            const FVector2D Dir(MoveDir.Y, MoveDir.X);
            MouseAim = Dir;
        }
    }
    else //커서가 밖인 경우
    {
        // 직전 방향을 유지해 미세 끊김(속도 손실)을 막는다.
        MoveDir = LastCursorDir;
    }

    // [이동]
    if (!MoveDir.IsNearlyZero() && bRightMouseHeld)
    {
        // 월드 방향 (dx,dy) → 기존 스틱 포맷 FVector2D(Y=dx, X=dy)
        MouseMove = FVector2D(MoveDir.Y, MoveDir.X);
    }

    // [조준] : MouseAim 설정 및 CursorPoint 비활성화
    if (bLeftMouseHeld && bHasLastCursorPoint) //양심상 커서 위치는 있어야지.
    {
        FVector ToAim = LastCursorPoint - GetActorLocation();
        ToAim.Z = 0;

        //정규화
        ToAim.Normalize();

        MouseAim = FVector2D(ToAim.Y, ToAim.X);
        SetDesiredAimPoint(LastCursorPoint);
    }
}

//우클릭시 커서 위치 땅 값을 반환하고 땅이 있는지
bool AMyPlayer::GetCursorGroundLocation(FVector& OutLocation) const
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return false;
    }

    // 화면의 마우스 위치를 월드 광선(원점+방향)으로 역투영
    FVector WorldOrigin, WorldDirection;

    // 커서가 화면 밖이거나 마우스 위치 없음
    if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
    {
        return false; 
    }

    // 플레이어 발 높이의 수평면(z = 플레이어 Z)과 카메라 광선의 교점을 구함.
    // 밟을 수 있는 지형인지.
    if (FMath::IsNearlyZero(WorldDirection.Z))
    {
        return false; // 광선이 수평이면 평면과 안 만남
    }
    const float T = (GetActorLocation().Z - WorldOrigin.Z) / WorldDirection.Z;

    // 평면이 카메라 뒤쪽
    if (T < 0.0f)
    {
        return false; 
    }
    OutLocation = WorldOrigin + WorldDirection * T; //out cursor position
    return true;
}



void AMyPlayer::UpdateMovement(float DeltaTime, const FVector2D& Move)
{
    if (Move.SizeSquared() <= InputDeadzone * InputDeadzone)
    {
        return;
    }

    // 카메라가 고정(yaw 0)이므로 월드축 기준: 스틱 위(+Y) = 화면 위(+X), 스틱 오른쪽(+X) = +Y
    AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Move.Y);
    AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Move.X);
}

void AMyPlayer::UpdateAimAndAttack(float DeltaTime, const FVector2D& Aim, const FVector2D& Move)
{
    const bool bAiming = Aim.SizeSquared() > InputDeadzone * InputDeadzone;

    AttackFacingHold = FMath::Max(0.0f, AttackFacingHold - DeltaTime);

    //조준
    if (bAiming)
    {
        const FVector AimDir(Aim.Y, Aim.X, 0.0f); // 이동과 동일한 축 매핑
        SetActorRotation(FRotator(0.0f, AimDir.Rotation().Yaw, 0.0f));
    }
    // 공격중
    else if (AttackFacingHold > 0.0f)
    {
        // 공격위치 바라보기
        SetActorRotation(FRotator(0.0f, GetAttackAimDir().Rotation().Yaw, 0.0f));
    }
    // 공격이 끝나면
    else if (Move.SizeSquared() > InputDeadzone * InputDeadzone)
    {
        // 이동 방향으로 되돌리기
        const FVector MoveDir(Move.Y, Move.X, 0.0f);
        const FRotator TargetRot(0.0f, MoveDir.Rotation().Yaw, 0.0f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnInterpSpeed));
    }

    // 조준 중에는 일정 간격으로 자동 공격. 타이머·간격·직업 호출은 베이스가 처리하고,
    // 플레이어는 "지금 공격하고 싶은가"(= 조준 중인가)만 넘긴다.
    if (TickAttack(DeltaTime, bAiming))
    {
        AttackFacingHold = GetAttackMontageLength();
    }
}

// 공격 중에는 클릭한 방향을 유지해야 해서, 그 유지 시간을 몽타주 길이로 잡는다.
float AMyPlayer::GetAttackMontageLength()
{
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
    if (!Montage || Montage->GetPlayLength() <= 0.0f)
    {
        return AttackFacingHoldFallback;
    }

    return Montage->GetPlayLength();
}

// 다리(하체)를 실제 이동 방향으로 돌리기 위한 yaw 오프셋을 계산한다.
// 액터(상체/조준)는 조준 방향을 보고 있으므로, 속도 방향과 액터 회전의 차이가 곧 "다리를 더 돌려야 할 각도"다.
// AnimBP가 이 값으로 pelvis(+)·spine_01(-)을 회전시켜 다리만 이동 방향을 향하게 한다.
void AMyPlayer::UpdateLegYawOffset(float DeltaTime)
{
    FVector Vel = GetVelocity();
    Vel.Z = 0.0f;

    // 거의 정지 상태면 다리를 몸과 정렬(오프셋 0)로 부드럽게 되돌린다 → 서서 조준하면 다리도 정면.
    float TargetOffset = 0.0f;
    if (Vel.SizeSquared() > 1.0f)
    {
        const float MoveYaw = Vel.Rotation().Yaw;
        const float ActorYaw = GetActorRotation().Yaw;
        // 액터 정면 기준 이동 방향의 상대 각도(-180~180). 전방 애니 1개라 허리 꺾임 방지로 ±Max로 제한.
        TargetOffset = FMath::FindDeltaAngleDegrees(ActorYaw, MoveYaw);
        TargetOffset = FMath::Clamp(TargetOffset, -LegYawMaxAngle, LegYawMaxAngle);
    }

    LegYawOffset = FMath::FInterpTo(LegYawOffset, TargetOffset, DeltaTime, LegYawInterpSpeed);
}







void AMyPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 게임패드: 별도 에셋 없이 아날로그 스틱 키에 직접 바인딩
    // (왼쪽 스틱 = 이동, 오른쪽 스틱 = 조준+공격)
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftX, this, &AMyPlayer::OnMoveX);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftY, this, &AMyPlayer::OnMoveY);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightX, this, &AMyPlayer::OnAimX);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightY, this, &AMyPlayer::OnAimY);

    // 마우스(로스트아크식): 좌클릭 = 공격, 우클릭 = 이동. 누르는 동안 Tick에서 처리.
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AMyPlayer::OnLeftMousePressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AMyPlayer::OnLeftMouseReleased);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AMyPlayer::OnRightMousePressed);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AMyPlayer::OnRightMouseReleased);

    // 디버그/테스트: C 키로 돈 획득(AddMoney). bDebugAddMoneyKey로 토글.
    if (bDebugAddMoneyKey)
    {
        PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AMyPlayer::AddMoney);
    }
}

void AMyPlayer::ShowOnItemText(FText & Text, EItemNotifyType Type)
{
    if (!CanvasWidget)
    {
        return;
    }
    CanvasWidget->AddItemNotification(Text, Type);
}






void AMyPlayer::SetMoveInput(FVector2D Value) { TouchMove = Value; }
void AMyPlayer::SetAimInput(FVector2D Value) { TouchAim = Value; }

void AMyPlayer::OnMoveJoystickMoved(FVector2D Value) { SetMoveInput(Value); }
void AMyPlayer::OnAimJoystickMoved(FVector2D Value) { SetAimInput(Value); }

void AMyPlayer::OnMoveX(float Value) { GamepadMove.X = Value; }
void AMyPlayer::OnMoveY(float Value) { GamepadMove.Y = Value; }
void AMyPlayer::OnAimX(float Value) { GamepadAim.X = Value; }
void AMyPlayer::OnAimY(float Value) { GamepadAim.Y = Value; }

// 마우스 버튼: 누르고 있는 동안만 작동(로스트아크식 조작)
void AMyPlayer::OnLeftMousePressed() { bLeftMouseHeld = true; }
void AMyPlayer::OnLeftMouseReleased() { bLeftMouseHeld = false; }
void AMyPlayer::OnRightMousePressed() { bRightMouseHeld = true; }
void AMyPlayer::OnRightMouseReleased() { bRightMouseHeld = false; }




//////////////////////////////////      Property        //////////////////////////
void AMyPlayer::AddMoney()
{
    SetMoney(Money + 1);
}

void AMyPlayer::SetMoney(int value)
{
    this->Money = value;

    if (CanvasWidget)
    {
        CanvasWidget->UpdateCoinText(this->Money);
    }

}

bool AMyPlayer::TrySpendMoney(int32 Amount)
{
    if (Amount <= 0)
    {
        return true; // 비용이 0 이하면 그냥 통과
    }
    if (Money < Amount)
    {
        return false; // 돈 부족 → 아무것도 안 함
    }
    SetMoney(Money - Amount); // 차감 + 코인 UI 갱신
    return true;
}

void AMyPlayer::AddExp(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    Exp += Amount;

    // 한 번에 여러 레벨을 오를 수도 있다(큰 보상). 필요량을 빼고 남는 경험치는 이월.
    while (Exp >= GetExpToNextLevel())
    {
        Exp -= GetExpToNextLevel();
        Level++;
        OnLevelUp(Level); // BP: 이펙트/사운드/스탯 상승
    }

    UpdateExpUI();
}

int32 AMyPlayer::GetExpToNextLevel() const
{
    return ExpBase + (Level - 1) * ExpGrowth;
}

void AMyPlayer::UpgradeWeapon()
{
    WeaponLevel++;

    // 데미지는 직업 컴포넌트가 소유한다(검사 근접/궁수·마법사 발사체 모두 CurrentJob->Damage 사용).
    if (CurrentJob)
    {
        CurrentJob->BonusDamage += WeaponDamagePerLevel;
    }

    OnWeaponUpgraded(WeaponLevel); // BP: 이펙트/사운드/무기 외형 교체

}

void AMyPlayer::UpdateExpUI()
{
    if (CanvasWidget)
    {
        CanvasWidget->UpdateExp(Level, Exp, GetExpToNextLevel());
    }
}

// HP바 폭 갱신. 공식이 여기 한 곳에만 살게 해서 호출부마다 달라지는 일을 막는다.
// (MaxHP가 0이면 정수 나눗셈으로 크래시하므로 최소 1로 막는다)
void AMyPlayer::UpdateHPUI()
{
    if (CanvasWidget)
    {
        CanvasWidget->SetProgressUISize(FVector2D(HP * 500 / FMath::Max(1, MaxHP), 50));
    }
}

void AMyPlayer::SetHP(int32 new_hp)
{
    // HP 대입 + IsDead 전환 + OnDeath/OnRevive 호출은 베이스가 담당.
    Super::SetHP(new_hp);

    // HUD 체력바 갱신
    if (CanvasWidget)
    {
        UpdateHPUI();

        // 사망 패널을 현재 죽음 상태와 동기화.
        // OnDeath/OnRevive는 "전환 시점"에만 1회 호출이라, BP BeginPlay(위젯 연결)가
        // ReStart()보다 먼저 돌면서 HP=0으로 패널이 켜진 경우 아무도 안 꺼주는 빈틈이 있다.
        // 여기서 매번 맞춰주면 호출 순서와 무관하게 패널 상태가 항상 올바르다.
        CanvasWidget->ShowDeathPanel(IsDead);
    }

    // BP 이벤트에 현재 죽음 상태 통보(사망 연출 등)
    CheckDeath(IsDead);
}

bool AMyPlayer::GetIsDead()
{
    // 조회 전용 — 예전처럼 이동/입력 모드를 건드리지 않는다(그건 OnDeath/OnRevive가 전환 시 1회씩).
    return IsDead;
}

// 살아있음 → 죽음 전환 시 베이스(SetDead)가 1회 호출.
void AMyPlayer::OnDeath()
{
    // 진행 중인 공격 몽타주를 즉시 끊어 슬롯을 비움 → ABP의 죽음 상태가 바로 재생됨
    StopAnimMontage();

    GetCharacterMovement()->DisableMovement();

    // 디아블로식 사망 연출 — 적들의 시간을 멈춘다(플레이어 죽음 애니메이션은 그대로 재생).
    if (AZombieSlayerGameMode* GM = GetWorld()->GetAuthGameMode<AZombieSlayerGameMode>())
    {
        GM->SetEnemiesFrozen(true);
    }

    if (CanvasWidget)
    {
        CanvasWidget->ShowDeathPanel(true); //사망 패널 on (BP에 DeathPanel이 있을 때만)
    }

    // 죽으면 UI(사망 패널)만 조작 가능
    if (PlayerControllerRef)
    {
        PlayerControllerRef->bShowMouseCursor = true;
        PlayerControllerRef->SetInputMode(FInputModeUIOnly());
    }
}

// 죽음 → 부활(ReStart의 SetHP) 전환 시 베이스가 1회 호출. 죽을 때 껐던 것을 되돌린다.
void AMyPlayer::OnRevive()
{
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // 죽을 때 멈춘 적들의 시간을 다시 흐르게 한다.
    if (AZombieSlayerGameMode* GM = GetWorld()->GetAuthGameMode<AZombieSlayerGameMode>())
    {
        GM->SetEnemiesFrozen(false);
    }

    if (CanvasWidget)
    {
        CanvasWidget->ShowDeathPanel(false); //사망 패널 off
    }

    if (PlayerControllerRef)
    {
        // 마우스 커서를 보이게 하고, 게임+UI 입력 모드로 둠
        // → 데스크탑에서 마우스로 조이스틱을 조작할 수 있고 커서도 보임
        PlayerControllerRef->bShowMouseCursor = true;

        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PlayerControllerRef->SetInputMode(InputMode);
    }
}

//...?
void AMyPlayer::ReStart()
{
    SetHP(MaxHP);
    SetMoney(0);

    // 경험치/레벨도 초기화 (돈과 동일한 재시작 규칙). 죽어도 유지하고 싶으면 이 두 줄만 지우면 된다.
    Level = 1;
    Exp = 0;
    UpdateExpUI();

    // 무기 강화도 초기화 — 강화로 올린 만큼만 되돌린다(직업 기본 Damage는 보존). 유지하고 싶으면 삭제.
    if (CurrentJob && WeaponLevel > 0)
    {
        CurrentJob->BonusDamage -= WeaponLevel * WeaponDamagePerLevel;
    }
    WeaponLevel = 0;
    //리스폰
    if (playerStart)
    {
        SetActorLocation(playerStart->GetActorLocation());
    }
}

//모바일용
void AMyPlayer::MoveTopDown(FVector2D Value)
{
    // 카메라가 yaw 0으로 고정(월드 +X를 바라봄)이므로 컨트롤러 회전을 무시하고 월드축으로 이동.
    // UpdateMovement(게임패드)와 동일한 매핑: 스틱/키 위(+Y) = 화면 위(+X), 오른쪽(+X) = +Y
    AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value.Y);
    AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Value.X);
}

// HandleAttackNotify는 베이스(ACombatCharacter)의 기본 구현이 그대로 처리한다
// (현재 직업의 OnAttackNotify로 전달) — 플레이어만의 동작이 아니라 동료와 동일해서 올렸다.

void AMyPlayer::SetCanvasWidget(UMyCanvas* CW)
{
    CanvasWidget = CW;

    // 캔버스에 배치된 조이스틱의 입력을 내 콜백에 바인딩.
    // (플레이어가 캔버스를 들고 있으므로 캔버스가 거꾸로 플레이어를 찾을 필요 없음)
    if (CanvasWidget)
    {
        // 모바일(안드로이드/iOS)에서만 터치 조이스틱을 표시한다. PC에선 숨겨서
        // 마우스(로스트아크식) 조작만 쓰고, 클릭이 조이스틱 위젯에 먹히지 않게 한다.
        // Collapsed = 화면에서 빠지고 히트테스트도 안 됨 → 마우스 클릭이 게임으로 전달.
        const ESlateVisibility JoystickVis =
            IsMobilePlatform() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

        if (CanvasWidget->MoveJoystick)
        {
            CanvasWidget->MoveJoystick->SetVisibility(JoystickVis);
            CanvasWidget->MoveJoystick->OnJoystickMoved.AddUniqueDynamic(this, &AMyPlayer::OnMoveJoystickMoved);
        }
        if (CanvasWidget->AimJoystick)
        {
            CanvasWidget->AimJoystick->SetVisibility(JoystickVis);
            CanvasWidget->AimJoystick->OnJoystickMoved.AddUniqueDynamic(this, &AMyPlayer::OnAimJoystickMoved);
        }

        // 위젯이 막 연결된 시점에 현재 HP 기준으로 사망 UI/HP바를 동기화한다.
        // BeginPlay 때는 CanvasWidget이 아직 null이라 패널을 못 껐을 수 있으므로 여기서 확정.
        CanvasWidget->ShowDeathPanel(HP <= 0);
        UpdateHPUI();
        UpdateExpUI(); // 위젯 연결 시점에 경험치 표시도 현재 값으로 맞춘다
    }
}


// [Debug]
// 속도 체크
void AMyPlayer::DebugWalkSpeed(const FVector2D& Move)
{
    // 속도 튜닝용 디버그(토글). 입력 크기 / 실제 속도 / 최고속도를 화면에 출력.
    if (bShowSpeedDebug && GEngine)
    {
        const float MaxSpd = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.0f;
        GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Green,
            FString::Printf(TEXT("Move=%.2f  Vel=%.0f  MaxWalkSpeed=%.0f"),
                Move.Size(), GetVelocity().Size(), MaxSpd));
    }
}
