
#include "LevelLoaderSubsystem.h"
#include "UI/LoadingWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"

// 생성자
void ULevelLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	
	// 레벨 전환이 끝나면 이 함수를 알려줘 => 로딩 화면 제거
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ULevelLoaderSubsystem::OnPostLoadMap);
}

// 소멸자
void ULevelLoaderSubsystem::Deinitialize()
{
	//이 함수를 이제 알려주지마. => 옵저버 끊기
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	UnregisterTick();

	Super::Deinitialize();
}

// TSubclassOf<ULoadingWidget> LoadingWidgetClass,
// 비동기 로드 (핵심 함수) => 여담으로 여기는 default 매개변수 적으면 안된다.
// MinDisplayTime : 최소 몇 초는 보여줘라
void ULevelLoaderSubsystem::LoadLevelAynsc(TSoftObjectPtr<UWorld> Level,
	TSubclassOf<ULoadingWidget> LoadingWidgetClass,
	float MinDisplayTime)
{
	// 로딩이 됐다면. 또는 로딩중인 레벨이 안 나왔다면
	if (bIsLoading || Level.IsNull())
	{
		return;
	}

	// 설정
	PendingLevel = Level;
	PendingPackageName = FName(*Level.ToSoftObjectPath().GetLongPackageName());
	bIsLoading = true;
    bPackageLoaded = false;

	MinTime = MinDisplayTime;
	Elapsed = 0.f;
	DisplayProgress = 0.f;

	// todo 로딩 화면
	if (LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<ULoadingWidget>(GetGameInstance(), LoadingWidgetClass);
		if (LoadingWidget)
		{
			LoadingWidget->AddToViewport(100);
		}
	}

	// 비동기 로드
	LoadPackageAsync(PendingPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &ULevelLoaderSubsystem::OnPackageLoaded));

	// Tick 설정 => 비동기 ing 함수
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ULevelLoaderSubsystem::Tick));
}

//로드 됐다면 => 패키지를 로드하는 느낌
void ULevelLoaderSubsystem::OnPackageLoaded(const FName & PackageName, UPackage * LoadedPackage, EAsyncLoadingResult::Type Result)
{
	//성공하지 못했다면
	if (Result != EAsyncLoadingResult::Succeeded || !LoadedPackage)
	{
		UE_LOG(LogTemp, Error, TEXT("[LevelLoader] 로드 실패: %s"), *PackageName.ToString());
		Finish();
		return;
	}

	LoadedMapPackage = LoadedPackage;
	bPackageLoaded = true;
}

bool ULevelLoaderSubsystem::Tick(float DeltaTime)
{
	// 진행 업데이트
	Elapsed += DeltaTime;

	// 진행률
	float Process = 0.95f;
	if (bPackageLoaded)
	{
		Process = 1.0f;
	}
	else
	{
		const float RealProcess = GetAsyncLoadPercentage(PendingPackageName);

		// PendingPackageName가 추적이 가능하다면
		if (RealProcess >= 0.f)
		{
			Process = FMath::Min(Process, RealProcess / 100.f);
		}
	}
	// 선형 보간
	DisplayProgress = FMath::FInterpConstantTo(DisplayProgress, Process, DeltaTime, 1.0f);

	// 전환
	if (bPackageLoaded && DisplayProgress >= 1.0f && Elapsed >= MinTime)
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), PendingLevel);
		TickHandle.Reset();
		return false;
	}

	return true;
}

// 로드가 끝난 뒤. 
void ULevelLoaderSubsystem::OnPostLoadMap(UWorld * LoadedWorld)
{
	// 로딩이 됐다면
	if (bIsLoading)
	{
		Finish();
	}
}

// 로딩바 치우는 함수. 맵 제거는 안한다.
void ULevelLoaderSubsystem::Finish()
{
	UnregisterTick();

	bIsLoading = false;
}

void ULevelLoaderSubsystem::UnregisterTick()
{
	// 왜 확인하는가? 사실 이유는 없다.
	// 이게 관례기도 하고 사실 api, 엔진 업데이트같은 자잘한 것 때문에 오류 생기는 거 방지
	if (TickHandle.IsValid())
	{
		// 만약 로딩이 다 안됐는데 강제 종료하면 제거 함수
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	LoadedMapPackage = nullptr;
	PendingLevel.Reset();
	PendingPackageName = NAME_None;
	bIsLoading = false;
}

