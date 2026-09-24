# 답안지 — 비동기 레벨 로드 (유니티 LoadSceneAsync 대응)

## 현재 상태

레벨 이동이 전부 **동기**(`OpenLevel`)다. 로딩 동안 화면이 멈춘다.

| 위치 | 이동 | 방식 |
|---|---|---|
| `WBP_MainMenuCanvas` GamePlayButton | MainMenu → GameReady | BP `Open Level (by Object Reference)` |
| `WBP_GameReadyCanvas` GamePlayButton | GameReady → GamePlay | BP `Open Level (by Object Reference)` |
| `UDeathPanelWidget::OnMainMenuClicked` | GamePlay → MainMenu | C++ `UGameplayStatics::OpenLevel` |

## 유니티 ↔ 언리얼 대응

| 유니티 | 언리얼 (이 답안) |
|---|---|
| `SceneManager.LoadSceneAsync(name)` | `LoadPackageAsync(맵 패키지 경로, 완료 콜백)` |
| `AsyncOperation.progress` | `GetAsyncLoadPercentage(패키지명)` (0~100, 모르면 -1) |
| `allowSceneActivation = false` | 로드 끝나도 `OpenLevel`을 **늦게 부르면** 된다 |
| `allowSceneActivation = true` | `OpenLevel` 호출 — 패키지가 이미 메모리에 있어서 거의 즉시 |
| `DontDestroyOnLoad` 로딩 매니저 | `UGameInstanceSubsystem` (게임 내내 1개, 레벨 바뀌어도 삶) |

핵심 아이디어: **무거운 부분(맵 에셋 로드)을 먼저 비동기로 끝내놓고, 그다음 `OpenLevel`을 부른다.**
`OpenLevel` 자체는 여전히 동기지만, 이미 로드된 패키지를 찾아 쓰므로 멈춤이 짧다.

주의할 점:
- 로드된 패키지를 `UPROPERTY`로 잡아두지 않으면, `OpenLevel` 도중 GC가 돌면서 버려질 수 있다 → `LoadedMapPackage`로 붙잡는다.
- `GetAsyncLoadPercentage`는 로더 구현(Zen/IoStore)에 따라 **-1만 돌려줄 수 있다**. 그래서 -1이면 90%까지 천천히 차는 가짜 진행률로 대체한다.
- PIE에선 맵 패키지 이름에 `UEDPIE_0_` 접두사가 붙어 미리 로드한 패키지가 재사용되지 않을 수 있다. 효과 체감은 **Standalone Game**으로 확인.

작업 순서: 1 → 2 → 3 → 4 → 5 → 6 → **풀 리빌드** (새 클래스 2개라 Live Coding 금지) → 7 → 8 → 9 → Standalone 확인

```
ULevelLoaderSubsystem (GameInstanceSubsystem)   로드 시작 / 진행률 / 전환 / 정리
ULoadingWidget (UserWidget)                       진행률만 읽어서 ProgressBar에 표시
 └─ WBP_Loading                                    생김새 (디자이너)
```

---

## 1. 로더 서브시스템 헤더 — Source/ZombieHunter1/Public/LevelLoaderSubsystem.h (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "UObject/UObjectGlobals.h"
#include "LevelLoaderSubsystem.generated.h"

class UPackage;
class UWorld;
class ULoadingWidget;

// 유니티 SceneManager.LoadSceneAsync 대응 => 자세한 내용은 노션 개발문서 참고
UCLASS()
class ZOMBIEHUNTER1_API ULevelLoaderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadLevelAsync(TSoftObjectPtr<UWorld> Level, TSubclassOf<ULoadingWidget> LoadingWidgetClass, float MinDisplayTime = 1.0f);

	// 0~1
	UFUNCTION(BlueprintPure, Category = "Level")
	float GetProgress() const { return DisplayProgress; }

	UFUNCTION(BlueprintPure, Category = "Level")
	bool IsLoading() const { return bIsLoading; }

private:
	void OnPackageLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result);
	bool Tick(float DeltaTime);
	void OnPostLoadMap(UWorld* LoadedWorld);
	void Finish();

	// [상태]
	TSoftObjectPtr<UWorld> PendingLevel;
	FName PendingPackageName;
	bool bIsLoading = false;
	bool bPackageLoaded = false;
	float MinTime = 0.f;
	float Elapsed = 0.f;
	float DisplayProgress = 0.f;

	// [참조 유지]
	UPROPERTY()
	TObjectPtr<UPackage> LoadedMapPackage;

	UPROPERTY()
	TObjectPtr<ULoadingWidget> LoadingWidget;

	// [핸들]
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle PostLoadMapHandle;
};
```

## 2. 로더 서브시스템 구현 — Source/ZombieHunter1/Private/LevelLoaderSubsystem.cpp (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelLoaderSubsystem.h"
#include "UI/LoadingWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"

void ULevelLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULevelLoaderSubsystem::OnPostLoadMap);
}

void ULevelLoaderSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	Super::Deinitialize();
}

void ULevelLoaderSubsystem::LoadLevelAsync(TSoftObjectPtr<UWorld> Level, TSubclassOf<ULoadingWidget> LoadingWidgetClass, float MinDisplayTime)
{
	if (bIsLoading || Level.IsNull())
	{
		return;
	}

	PendingLevel = Level;
	PendingPackageName = FName(*Level.ToSoftObjectPath().GetLongPackageName());   // => "/Game/Maps/GameReady"
	bIsLoading = true;
	bPackageLoaded = false;
	MinTime = MinDisplayTime;
	Elapsed = 0.f;
	DisplayProgress = 0.f;

	// [로딩 화면]
	if (LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<ULoadingWidget>(GetGameInstance(), LoadingWidgetClass);
		if (LoadingWidget)
		{
			LoadingWidget->AddToViewport(100);
		}
	}

	// [비동기 로드]
	LoadPackageAsync(PendingPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &ULevelLoaderSubsystem::OnPackageLoaded));

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ULevelLoaderSubsystem::Tick));
}

void ULevelLoaderSubsystem::OnPackageLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
{
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
	Elapsed += DeltaTime;

	// [진행률]
	float Target = 0.9f;
	if (bPackageLoaded)
	{
		Target = 1.f;
	}
	else
	{
		const float Real = GetAsyncLoadPercentage(PendingPackageName);   // 0~100, 모르면 -1
		if (Real >= 0.f)
		{
			Target = FMath::Min(Real / 100.f, 0.9f);
		}
	}
	DisplayProgress = FMath::FInterpConstantTo(DisplayProgress, Target, DeltaTime, 1.f);

	// [전환] = allowSceneActivation
	if (bPackageLoaded && DisplayProgress >= 1.f && Elapsed >= MinTime)
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), PendingLevel);
		TickHandle.Reset();
		return false;
	}

	return true;
}

void ULevelLoaderSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (bIsLoading)
	{
		Finish();
	}
}

void ULevelLoaderSubsystem::Finish()
{
	if (TickHandle.IsValid())
	{
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
```

## 3. 로딩 위젯 헤더 — Source/ZombieHunter1/Public/UI/LoadingWidget.h (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;

// WBP_Loading의 부모
UCLASS()
class ZOMBIEHUNTER1_API ULoadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* LoadingBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* PercentText;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
```

## 4. 로딩 위젯 구현 — Source/ZombieHunter1/Private/UI/LoadingWidget.cpp (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LoadingWidget.h"
#include "LevelLoaderSubsystem.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void ULoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UGameInstance* GI = GetGameInstance();
	const ULevelLoaderSubsystem* Loader = GI ? GI->GetSubsystem<ULevelLoaderSubsystem>() : nullptr;
	if (!Loader)
	{
		return;
	}

	const float Progress = Loader->GetProgress();

	if (LoadingBar)
	{
		LoadingBar->SetPercent(Progress);
	}

	if (PercentText)
	{
		PercentText->SetText(FText::AsPercent(Progress));   // => 0.42 → "42%"
	}
}
```

## 5. 사망 패널 헤더: FName → 소프트 레퍼런스 — Source/ZombieHunter1/Public/UI/DeathPanelWidget.h:9, 31~33

```cpp
// Before
class UButton;
```
```cpp
class UButton;
class ULoadingWidget;
```

```cpp
// Before
	/** MainMenuButton이 여는 맵 이름. WBP 디폴트에서 바꿀 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FName MainMenuLevelName = TEXT("MainMenu");
```
```cpp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	TSoftObjectPtr<UWorld> MainMenuLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/MainMenu.MainMenu")));

	// 기본값: WBP_Loading
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	TSubclassOf<ULoadingWidget> LoadingWidgetClass;
```

## 6. 사망 패널 구현: OpenLevel → 로더 — Source/ZombieHunter1/Private/UI/DeathPanelWidget.cpp:4~6, 18~22

```cpp
// Before
#include "UI/DeathPanelWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
```
```cpp
#include "UI/DeathPanelWidget.h"
#include "UI/LoadingWidget.h"
#include "LevelLoaderSubsystem.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
```

```cpp
// Before
void UDeathPanelWidget::OnMainMenuClicked()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
```
```cpp
void UDeathPanelWidget::OnMainMenuClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelLoaderSubsystem* Loader = GI->GetSubsystem<ULevelLoaderSubsystem>())
		{
			Loader->LoadLevelAsync(MainMenuLevel, LoadingWidgetClass);
		}
	}
}
```

---

**↑ 여기까지 하고 풀 리빌드** (에디터 끄고 VS에서 빌드)

---

## 7. WBP_Loading 만들기 — Content/UI/BP/WBP_Loading (에디터)

1. Content/UI/BP 우클릭 → User Interface → Widget Blueprint → 이름 `WBP_Loading`
2. Class Settings → Parent Class = `LoadingWidget`
3. 디자이너 계층:
   ```
   [Canvas Panel]
    ├─ Image (앵커: 전체 채움, Offset 전부 0, 색 검정)
    ├─ ProgressBar  이름: LoadingBar   (필수 — 이름 틀리면 컴파일 에러)
    └─ TextBlock    이름: PercentText  (선택)
   ```
4. 컴파일 → 저장

## 8. 메뉴 BP 두 곳 교체 — WBP_MainMenuCanvas / WBP_GameReadyCanvas 이벤트 그래프

두 WBP 모두 `On Clicked (GamePlayButton)` 뒤의 **`Open Level (by Object Reference)` 노드를 지우고** 아래로 바꾼다.

```
On Clicked (GamePlayButton)
  → [Get Game Instance Subsystem]  Class = LevelLoaderSubsystem
  → [Load Level Async]  Target = 위 반환값
       Level               = GameReady   (GameReadyCanvas에선 GamePlay)
       Loading Widget Class = WBP_Loading
       Min Display Time     = 1.0
```

`WBP_GameReadyCanvas`는 기존 `Set Selected Job Class` 등은 그대로 두고, 맨 끝의 Open Level만 바꾼다.

## 9. WBP_DeathPanel 디폴트 — Class Defaults

- `Loading Widget Class` = `WBP_Loading`
- `Main Menu Level` = `MainMenu` 로 들어가 있는지 확인

---

## 확인

- Play 드롭다운 → **Standalone Game** 으로 실행
- 메인메뉴 → Game 버튼: 검은 화면 + 바가 0→100% 찬 뒤 GameReady로 넘어가야 함
- 로딩 중 버튼 연타해도 두 번 로드되지 않아야 함 (`bIsLoading` 가드)
- 출력 로그에 `[LevelLoader] 로드 실패`가 뜨면 Level 핀 값 확인

## 다음 단계 (선택)

`OpenLevel`의 마지막 짧은 멈춤(월드 초기화·BeginPlay)까지 가리려면 `MoviePlayer` 모듈의 로딩 스크린(`GetMoviePlayer()->SetupLoadingScreen`)을 `PreLoadMap`에 걸어야 한다. 이건 별도 답안지로.

---

## 트러블슈팅 — ULoadingWidget `GetPrivateStaticClass` 등 대량 에러 (2026-09-24)

원인: `LoadingWidget.generated.h`가 **옛날 헤더 기준으로 생성된 채 남아 있음.**

- `GENERATED_BODY()`는 `현재파일ID_<줄번호>_GENERATED_BODY` 매크로로 펼쳐진다.
- 생성 파일엔 `..._LoadingWidget_h_12_GENERATED_BODY` (12번 줄 기준)만 정의돼 있다.
- 현재 헤더의 `GENERATED_BODY()`는 **10번 줄** (맨 위 주석 2줄을 지움) → `..._h_10_GENERATED_BODY`를 찾는데 없음 → 생성자/StaticClass 등이 전부 사라져 에러 폭발.
- 헤더와 generated.h 수정 시각이 같은 초(04:25:48)라 UHT가 "최신"으로 판단해 다시 만들지 않는다.

해결: 헤더를 generated.h보다 새것으로 만든 뒤 빌드.
- `LoadingWidget.h`에 빈 줄 하나 추가 → 저장 → 에디터 끄고 빌드, 또는
- `Intermediate/Build/Win64/UnrealEditor/Inc/ZombieHunter1/UHT/LoadingWidget.generated.h` / `.gen.cpp` 삭제 후 빌드

`FTextureBuildSettings` / `형식 지정자가 없습니다` 류는 IntelliSense 오류라 무시.
