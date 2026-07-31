// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/MyPlayer.h"
#include "Characters/Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h" //BindAxisKey
#include "InputCoreTypes.h"            //EKeys
#include "Kismet/GameplayStatics.h" //getCharacter, sound
#include "Animation/AnimInstance.h" //SetRootMotionMode, ERootMotionMode


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
#include "Characters/Companion.h" //동료 섭외
#include "Components/CapsuleComponent.h" //동료 스폰 시 캡슐 반높이
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

//생성자
AMyPlayer::AMyPlayer()
{
 	// Tick() 업데이트 키는 변수
	PrimaryActorTick.bCanEverTick = true;

	// 컨트롤러 회전이 캐릭터를 돌리지 않게 함 (조준 방향으로 직접 회전시킴)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//// 이동 방향 자동 회전 끄기 — 오른쪽 스틱(조준) 방향으로 수동 회전
	GetCharacterMovement()->bOrientRotationToMovement = false;

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

	// NavMesh 동적 생성 : 플레이어 주변에만 NavMesh를 깔고, 멀어지면 제거.
	// 무한 청크 맵의 시야 범위(약 ViewRadius*ChunkSize)를 덮도록 반경 설정.
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(7000.0f, 9000.0f);

	// 기본 직업: 전사. 에디터(BP)에서 DefaultJobClass를 바꾸면 다른 직업으로 시작한다.
	DefaultJobClass = UWarriorJob::StaticClass();
}



// Called when the game starts or when spawned
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

    ReplaceWeapon();


    //SetMoney(0);
    ReStart(); // 주의: 엔진 내장 APawn::Restart()가 아니라 우리 부활 함수(대문자 S). HP/돈 초기화 + 스타트 지점 이동.

    // 직업(Job) 컴포넌트 생성 — 시작 시 1개 고정.
    if (!DefaultJobClass)
    {
        DefaultJobClass = UWarriorJob::StaticClass();
    }

    SetJob();
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


// 무기는 ChildActorComponent(Weapon_BP) → BP_sword 액터 → 그 안의 SkeletalMeshComponent 구조다.
// 그 안쪽 메시 컴포넌트를 찾아 캐시한다(직업이 이 메시만 교체). 직업 생성보다 먼저 찾아둬야 함.
void AMyPlayer::ReplaceWeapon()
{
    TArray<UChildActorComponent*> ChildComps;
    GetComponents<UChildActorComponent>(ChildComps);
    for (UChildActorComponent* CAC : ChildComps)
    {
        if (!CAC)
        {
            continue;
        }
        AActor* Child = CAC->GetChildActor();
        if (!Child)
        {
            continue;
        }
        USkeletalMeshComponent* InnerMesh = Child->FindComponentByClass<USkeletalMeshComponent>();
        if (!InnerMesh)
        {
            continue;
        }
        // 'Weapon' 태그가 붙은 ChildActorComponent를 우선 사용, 없으면 첫 번째를 폴백으로.
        if (CAC->ComponentHasTag(WeaponComponentTag))
        {
            WeaponMeshComponent = InnerMesh;
            break;
        }
        if (!WeaponMeshComponent)
        {
            WeaponMeshComponent = InnerMesh;
        }
    }
    if (!WeaponMeshComponent && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange,
            TEXT("[Weapon] 무기 ChildActor(예: Weapon_BP) 안의 SkeletalMeshComponent를 못 찾음"));
    }
}

void AMyPlayer::SetJob()
{
    // 직업 선택 씬에서 고른 직업이 GameInstance에 실려 왔으면 그걸 우선 사용한다.
    // 선택이 없거나(None) GameInstance 클래스를 아직 지정 안 했으면(캐스트 실패)
    // 기존 DefaultJobClass 그대로 — 에디터 셋업 전에도 동작이 바뀌지 않는다.

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White,
            FString::Printf(TEXT("선택했어!")));
    }
    if (UZombieGameInstance* GI = Cast<UZombieGameInstance>(GetGameInstance()))
    {
        if (GI->SelectedJobClass)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White,
                    FString::Printf(TEXT("선택한 직업 : %s"), *GetNameSafe(GI->SelectedJobClass)));
            }
            DefaultJobClass = GI->SelectedJobClass;
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White,
            FString::Printf(TEXT("최종 결과 %s"), *GetNameSafe(DefaultJobClass)));
    }
    // 직업 컴포넌트 생성/부착은 베이스(ACombatCharacter)가 담당한다 — 동료와 동일한 경로.
    CreateJobComponent();

    if (CurrentJob)
    {
        // 공격 몽타주는 캐릭터(JobAttackMontages[JobName])가 소유 — 직업으로 승계하지 않는다.
        // 사운드만 직업이 비어 있을 때 플레이어 것을 물려준다.
        if (!CurrentJob->AttackSound)
        {
            CurrentJob->AttackSound = AttackSound;
        }
    }
}









// update 왠만하면 여기선 bp함수를 쓰지마라
void AMyPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 죽으면 이동/조준/공격 입력 처리를 멈춤 (죽은 뒤 공격 몽타주가 또 재생되는 것 방지)
    if (IsDead)
    {
        return;
    }

    // 마우스(로스트아크식)를 기존 스틱 포맷으로 변환:
    // 커서 방향 (dx,dy) → FVector2D(Y=dx, X=dy). UpdateMovement/UpdateAimAndAttack의 축 매핑과 일치.
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

    // 속도 튜닝용 디버그(토글). 입력 크기 / 실제 속도 / 최고속도를 화면에 출력.
    if (bShowSpeedDebug && GEngine)
    {
        const float MaxSpd = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.0f;
        GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Green,
            FString::Printf(TEXT("Move=%.2f  Vel=%.0f  MaxWalkSpeed=%.0f"),
                Move.Size(), GetVelocity().Size(), MaxSpd));
    }

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




///////////////////////////      Tick :: Move, AIM      /////////////////////
void AMyPlayer::MouseInput(FVector2D & MouseMove, FVector2D & MouseAim)
{
    if (bRightMouseHeld || bLeftMouseHeld)
    {
        // 이번 프레임의 커서 방향을 구한다.
        FVector MoveDir = FVector::ZeroVector;
        FVector Cursor;
        if (GetCursorGroundLocation(Cursor)) //커서 지면에 닿았는지 여부
        {
            FVector ToCursor = Cursor - GetActorLocation();
            ToCursor.Z = 0.0f;
            // 커서가 StopRadius보다 멀 때만 이동(가까우면 방향이 뒤집혀 진동하므로 정지).
            if (ToCursor.SizeSquared() > CursorStopRadius * CursorStopRadius && ToCursor.Normalize())
            {
                LastCursorDir = ToCursor; // 유효 방향 캐시
                MoveDir = ToCursor;
            }
        }
        else
        {
            // 커서→월드 변환이 실패한 프레임
            // 직전 방향을 유지해 미세 끊김(속도 손실)을 막는다.
            MoveDir = LastCursorDir;
        }

        if (!MoveDir.IsNearlyZero())
        {
            // 월드 방향 (dx,dy) → 기존 스틱 포맷 FVector2D(Y=dx, X=dy)
            const FVector2D Dir(MoveDir.Y, MoveDir.X);
            if (bRightMouseHeld) { MouseMove = Dir; }
            if (bLeftMouseHeld) { MouseAim = Dir; }
        }
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

    // 조준 중이면 조준 방향, 아니면 이동 방향을 바라봄
    const FVector2D Face = bAiming ? Aim : Move;

    if (Face.SizeSquared() > InputDeadzone * InputDeadzone)
    {
        const FVector FaceDir(Face.Y, Face.X, 0.0f); // 이동과 동일한 축 매핑
        const FRotator TargetRot(0.0f, FaceDir.Rotation().Yaw, 0.0f);
        if (bAiming)
        {
            // 조준(좌클릭/오른쪽 스틱) 중에는 즉시 그 방향을 바라본다.
            // 부드러운 보간을 쓰면 회전이 끝나기 전에 발사돼 화살이 중간 방향으로 나가므로,
            // 발사 프레임에 정면 = 커서 방향이 되도록 스냅한다. (발사체는 액터 정면으로 나감)
            SetActorRotation(TargetRot);
        }
        else
        {
            // 단순 이동 방향 바라보기는 부드럽게 보간
            const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnInterpSpeed);
            SetActorRotation(NewRot);
        }
    }

    // 조준 중에는 일정 간격으로 자동 공격. 타이머·간격·직업 호출은 베이스가 처리하고,
    // 플레이어는 "지금 공격하고 싶은가"(= 조준 중인가)만 넘긴다.
    TickAttack(DeltaTime, bAiming);
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








/////////////////////////////////////  Weapon   ////////////////////////////////////

void AMyPlayer::SetWeaponMesh(USkeletalMesh* NewMesh)
{
    if (!WeaponMeshComponent)
    {
        return; // 무기 컴포넌트를 못 찾았으면(태그 미설정) 그냥 통과
    }

    // 기존 무기 컴포넌트의 메시만 교체. 컴포넌트 자체는 유지되므로 BP 참조가 안 깨진다.
    WeaponMeshComponent->SetSkeletalMeshAsset(NewMesh);
    // 무기가 없는 직업(NewMesh == null)은 컴포넌트를 숨긴다.
    WeaponMeshComponent->SetVisibility(NewMesh != nullptr);
}




////////////////////////////////       Companion         ///////////////////
// 동료 섭외 — 플레이어 옆에 동료를 스폰해 따라다니며 싸우게 한다.
void AMyPlayer::RecruitCompanion(TSubclassOf<UJobComponent> JobComponent)
{
    UWorld* World = GetWorld();
    if (!CheckCompanion(World))
    {
        return;
    }

    const FTransform SpawnTM = SetSpawnTransformCompanion(World);

    // 지연 스폰: BeginPlay가 돌기 전에 Leader/직업을 세팅해야
    ACompanion* Companion = World->SpawnActorDeferred<ACompanion>(
        CompanionClass, SpawnTM, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (!Companion)
    {
        return;
    }

    Companion->Leader = this;

    if (GEngine)
    {
        UJobComponent* JobCDO = JobComponent.GetDefaultObject();
        const FString JobTypeStr = JobCDO ? UEnum::GetValueAsString(JobCDO->JobType) : TEXT("None(맵에 없음)");

        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("[Companion] JobComponent : %s"), *JobTypeStr));
    }
    Companion->DefaultJobClass = JobComponent; //직업 셋팅

    UGameplayStatics::FinishSpawningActor(Companion, SpawnTM);

    Companions.Add(Companion);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("[Companion] 동료 섭외 완료! (현재 %d명)"), Companions.Num()));
    }
}


///////////////////////////////////      Input    ////////////////////////////////
// 
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
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("CanvasWidget is null")));
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

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
            FString::Printf(TEXT("[Weapon] +%d강! (Damage %d)"),
                WeaponLevel, CurrentJob ? CurrentJob->GetDamage() : -1));
    }
}

void AMyPlayer::UpdateExpUI()
{
    if (CanvasWidget)
    {
        CanvasWidget->UpdateExp(Level, Exp, GetExpToNextLevel());
    }
}

void AMyPlayer::SetHP(int32 new_hp)
{
    // HP 대입 + IsDead 전환 + OnDeath/OnRevive 호출은 베이스가 담당.
    Super::SetHP(new_hp);

    // HUD 체력바 갱신
    if (CanvasWidget)
    {
        CanvasWidget->SetProgressUISize(FVector2D(HP * 500 /MaxHP, 50)); //500

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
        CanvasWidget->SetProgressUISize(FVector2D(HP * 100, 50));
        UpdateExpUI(); // 위젯 연결 시점에 경험치 표시도 현재 값으로 맞춘다
    }
}


//
bool AMyPlayer::CheckCompanion(UWorld* World)
{
    if (!CompanionClass)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
                TEXT("[Companion] CompanionClass가 비어있음 - BP_MyPlayer Details에서 BP_Companion 지정"));
        }
        return false;
    }

    if (!World)
    {
        return false;
    }

    // 죽어서 사라진 동료는 목록에서 제거한 뒤 인원 체크.
    Companions.RemoveAll([](const ACompanion* C) { return !IsValid(C); });
    if (Companions.Num() >= MaxCompanions)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
                FString::Printf(TEXT("[Companion] 최대 인원(%d명) 도달 - 더 섭외 불가"), MaxCompanions));
        }
        return false;
    }
    return true;
}

FTransform AMyPlayer::SetSpawnTransformCompanion(UWorld* World)
{
    // 플레이어 회전 기준으로 오프셋을 적용해 옆/뒤쪽에 스폰(여러 명이면 살짝씩 벌어지게).
    const FRotator SpawnRot = GetActorRotation();
    FVector Offset = CompanionSpawnOffset;
    Offset.Y += Companions.Num() * 80.0f; // 두 번째부터는 옆으로 더 벌려 겹침 방지
    FVector SpawnLoc = GetActorLocation() + SpawnRot.RotateVector(Offset);

    // 바닥에 맞춰 스폰 — 위에서 아래로 트레이스해 지면을 찾고, 그 위에 캡슐 반높이만큼 띄운다.
    // (플레이어 Z 그대로 쓰면 캡슐 높이 차이로 바닥에 끼이거나 허공에서 떨어져 안 보일 수 있음)
    const FVector TraceStart = SpawnLoc + FVector(0, 0, 200.0f);
    const FVector TraceEnd = SpawnLoc - FVector(0, 0, 1000.0f);
    FHitResult Hit;
    FCollisionQueryParams Q;
    Q.AddIgnoredActor(this);
    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Q))
    {
        const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f;
        SpawnLoc = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 2.0f);
    }

    FTransform SpawnTM(SpawnRot, SpawnLoc);
    return SpawnTM;
}
