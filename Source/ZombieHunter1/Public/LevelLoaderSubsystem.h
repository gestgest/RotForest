
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "UObject/UObjectGlobals.h"
#include "LevelLoaderSubsystem.generated.h"

class UPackage;
class UWorld;
class ULoadingWidget;

UCLASS()
class ZOMBIEHUNTER1_API ULevelLoaderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// TSubclassOf<ULoadingWidget> LoadingWidgetClass,
	UFUNCTION(BlueprintCallable, Category="Level")
	void LoadLevelAynsc(
		TSoftObjectPtr<UWorld> Level,
		float MinDisplayTime = 1.0f
	);

	float GetProgress() const { return DisplayProgress; }
	bool IsLoading() const { return bIsLoading; }

private:
	void OnPackageLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result);
	bool Tick(float DeltaTime);

	void OnPostLoadMap(UWorld* LoadedWorld);
	void Finish();
	void UnregisterTick();


	// [변수]
	TSoftObjectPtr<UWorld> PendingLevel; // 느슨한 포인터
	FName PendingPackageName;
	
	bool bIsLoading = false;
	bool bPackageLoaded = false;
	float MinTime = 0.f;
	float Elapsed = 0.f;
	float DisplayProgress = 0.f;

	UPROPERTY()
	TObjectPtr<UPackage> LoadedMapPackage;


	// [핸들]
	// Thread safe Ticker => 더 가벼운 콜백 시스템.
	// tick 호출 횟수가 빠르다.
	// 그래서 가벼운 거 넣어야 한다.
	FTSTicker::FDelegateHandle TickHandle;

	FDelegateHandle PostLoadMapHandle; //언리얼의 일반 멀티캐스트 딜리게이트
};
