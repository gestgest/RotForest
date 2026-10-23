// Fill out your copyright notice in the Description page of Project Settings.


#include "Zones/GaugeZone.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

#include "UI/ZoneGaugeWidget.h"

#include "Engine/Engine.h"

// Sets default values
AGaugeZone::AGaugeZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 박스 트리거를 루트로. 기본 크기는 사람이 올라설 만한 발판 정도.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// 시각화용 발판 메시(메시는 BP에서 지정). 충돌은 끔 — 트리거만 충돌을 담당.
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GaugeWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("GaugeWidget"));
	GaugeWidgetComp->SetupAttachment(RootComponent);
	GaugeWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	GaugeWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	GaugeWidgetComp->SetDrawSize(FVector2D(260.0f, 70.0f));
	GaugeWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AGaugeZone::BeginPlay()
{
	Super::BeginPlay();

	// 유니티의 OntriggerEnter, Exit 함수 설정
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGaugeZone::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AGaugeZone::OnTriggerEndOverlap);
	}

	UpdateGaugeWidget();
}

// Called every frame
void AGaugeZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 완성 후 쿨다운 진행
	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	}

	// 추적 중인 플레이어가 죽었거나 사라졌으면 정리
	if (!IsValid(CurrentPlayer))
	{
		CurrentPlayer = nullptr;
		bPlayerInside = false;
	}

	const bool bActive = !bConsumed && CooldownRemaining <= 0.0f;

	if (bPlayerInside && bActive && IsValid(CurrentPlayer))
	{
		// 발판 위에 서 있으면 일정 간격마다 "돈을 소비"해서 게이지를 채운다.
		Fill_Timer += DeltaTime;
		if (Fill_Timer >= Fill_Interval)
		{
			Fill_Timer = 0.0f;

			int32 Amount = 0;
			if (TryFillOnce(CurrentPlayer, Amount) && Amount > 0)
			{
				FilledAmount = FMath::Min(FilledAmount + Amount, RequiredAmount);
				Progress = FMath::Clamp((float)FilledAmount / (float)RequiredAmount, 0.0f, 1.0f);

				if (FilledAmount >= RequiredAmount)
				{
					CompleteZone();
				}

				UpdateGaugeWidget();
			}
			//else 는 TryFillOnce에
		}
	}
	else
	{
		// 발판에서 벗어나면 결제 타이머만 초기화한다. 이미 채운 게이지는 유지(낸 돈이 아까우니까).
		Fill_Timer = 0.0f;
	}
}


//스폰존에 들어간다면
void AGaugeZone::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	AMyPlayer* Player = Cast<AMyPlayer>(OtherActor);
	if (!Player)
	{
		return;
	}

	CurrentPlayer = Player;
	bPlayerInside = true;
	bFailNotified = false;

	OnPlayerEntered(Player);
}

//스폰존에 나간다면
void AGaugeZone::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (OtherActor != CurrentPlayer)
	{
		return;
	}

	// 추적하던 플레이어가 구역을 떠남 — 다른 플레이어가 아직 안에 있으면 그 사람으로 승계.
	CurrentPlayer = nullptr;
	bPlayerInside = false;

	if (TriggerBox)
	{
		TArray<AActor*> Overlapping;
		TriggerBox->GetOverlappingActors(Overlapping, AMyPlayer::StaticClass());
		for (AActor* A : Overlapping)
		{
			if (AMyPlayer* P = Cast<AMyPlayer>(A))
			{
				CurrentPlayer = P;
				bPlayerInside = true;
				
				OnPlayerExited(P);
				break;
			}
		}
	}

}

void AGaugeZone::CompleteZone()
{
	AMyPlayer* Payer = CurrentPlayer;

	// HandleZoneFilled가 RequiredAmount를 키워도 안전하도록 먼저 리셋한다.
	FilledAmount = 0;
	Progress = 0.0f;

	if (IsValid(Payer))
	{
		HandleZoneFilled(Payer);
	}

	if (bOneShot)
	{
		bConsumed = true;
	}
	else
	{
		CooldownRemaining = Cooldown;
	}
}

void AGaugeZone::UpdateGaugeWidget()
{

	if (!GaugeWidgetComp)
	{
		return;
	}

	if (UZoneGaugeWidget* Gauge = Cast<UZoneGaugeWidget>(GaugeWidgetComp->GetWidget()))
	{
		Gauge->SetPercent(Progress);
		Gauge->SetTitle(GetZoneTitle());
		Gauge->SetCost(RequiredAmount);
	}
}


// 상태 영속: 언로드 때 저장한 값을 재생성된 발판에 주입.
// 새 액터는 기본값으로 태어나므로, 게이지/비용/소진 여부에 "이전 삶의 기억"을 돌려준다.
void AGaugeZone::RestorePadState(int32 InFilledAmount, int32 InRequiredAmount, bool bInConsumed)
{
	RequiredAmount = FMath::Max(1, InRequiredAmount);
	FilledAmount = FMath::Clamp(InFilledAmount, 0, RequiredAmount);
	bConsumed = bInConsumed;

	Progress = (float)FilledAmount / (float)RequiredAmount;

	UpdateGaugeWidget();
}

// 한번 채우는데 실패했다면 메세지 전송 => 대체로 꽉 찬 경우
bool AGaugeZone::NotifyFailOnce(AMyPlayer * Player, const FText& Msg)
{
	if (bFailNotified || !Player)
	{
		return false;
	}

	bFailNotified = true;
	FText Text = Msg;
	Player->ShowOnItemText(Text, EItemNotifyType::Blocked);
	return true;
}
