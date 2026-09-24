// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelLoaderSubsystem.h"

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
    bPackageLoaded = true;

	MinTime = MinDisplayTime;
	Elapsed = 0.f;
	DisplayProgress = 0.f;

	// todo 로딩 화면

	// 비동기 로드
	LoadPackageAsync(PendingPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &ULevelLoaderSubsystem::OnPackageLoaded));

	// Tick 설정
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ULevelLoaderSubsystem::Tick));
}

//로드 됐다면
void ULevelLoaderSubsystem::OnPackageLoaded(const FName & PackageName, UPackage * LoadedPackage, EAsyncLoadingResult::Type Result)
{

}

bool ULevelLoaderSubsystem::Tick(float DeltaTime)
{
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
}

