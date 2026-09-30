# 답안지 — 0.1.1 핫픽스 (터치 점프 / 공격 방향 / 뒤로가기 / 디버그 선 / Sky Atmosphere)

작업 순서: 1~2 (에디터: 터치 점프) → 3~4 (공격 방향 코드) → 5~9 (뒤로가기 코드) → **풀 리빌드** (새 클래스 `ExitPanelWidget`) → 10~11 (에디터) → 패키징 → 폰 확인

> 1, 2번은 원인을 코드/에셋에서 확인한 내용이고, 기기에서 재현해 본 건 아니다. 고친 뒤 폰에서 꼭 확인.

---

## 1. 터치하면 점프 — 원인

- 플레이어 BP `BP_ElfArden`(부모 `AMyPlayer`)은 입력 매핑 `Characters/ElfArden/Input/IMC_Default`를 추가하고, `IA_Jump` 이벤트에서 `Jump`를 호출한다.
- `IMC_Default` 안에 **`Touch1` 키 매핑**이 남아 있다 (캐릭터 에셋/서드퍼슨 템플릿의 기본값). 조이스틱 위젯이 터치를 먹지 않는 빈 화면을 누르면 `Touch1` → `IA_Jump` → `Jump`.
- C++(`AMyPlayer`)에는 점프 코드가 없다. 전부 에셋 쪽 문제.

## 2. 터치 점프 — 고치기 (에디터, 코드 아님)

1. `Content/Characters/ElfArden/Input/IMC_Default` 열기
2. `IA_Jump` 항목 중 키가 **Touch 1**인 매핑을 삭제 (Space Bar, Gamepad Face Button Bottom은 그대로)
3. 저장
4. 같은 이름이 `Content/ThirdPerson/Input/IMC_Default`에도 있다. 이 프로젝트가 쓰는 건 위의 ElfArden 쪽이지만, 혹시 다른 BP가 이걸 쓰면 똑같이 삭제.

점프 자체가 필요 없으면 `IA_Jump` 매핑을 전부 지워도 된다.

---

## 3. 공격 방향 — 원인

```
ACombatCharacter::TickAttack        AttackAimDir = GetActorForwardVector()  ← 공격을 "시작하는 순간"에 고정
  └ 몽타주 재생 (활: 시위를 당기고 몽타주 끝에서 발사 / 전사: Notify 시점에 타격)
       └ GetAttackAimDir()          ← 고정해 둔 AttackAimDir를 반환
```

- 공격이 시작된 뒤 **몽타주가 끝날 때까지** 발사/타격 방향은 시작할 때의 값이다.
- 오른쪽으로 공격하던 중 조준 스틱을 아래로 돌리면, 몸(`SetActorRotation`)은 바로 아래를 보지만 이미 시작된 공격은 오른쪽으로 나간다. (다음 공격부터 아래)
- `bAiming`인 동안 `AttackAimDir`를 갱신하는 곳이 없다. 마우스는 `AttackAimPoint`를 써서 영향이 덜하다.

## 4. 공격 방향 — 조준 중엔 공격 방향도 같이 갱신 — Source/ZombieHunter1/Private/Characters/MyPlayer.cpp:388

```cpp
// Before
    //조준
    if (bAiming)
    {
        const FVector AimDir(Aim.Y, Aim.X, 0.0f); // 이동과 동일한 축 매핑
        SetActorRotation(FRotator(0.0f, AimDir.Rotation().Yaw, 0.0f));
    }
```
```cpp
    //조준
    if (bAiming)
    {
        const FVector AimDir(Aim.Y, Aim.X, 0.0f); // 이동과 동일한 축 매핑
        SetActorRotation(FRotator(0.0f, AimDir.Rotation().Yaw, 0.0f));
        AttackAimDir = AimDir.GetSafeNormal();
    }
```

`AttackAimDir`는 `ACombatCharacter`의 protected 변수라 `AMyPlayer`에서 바로 쓸 수 있다.
조준을 놓으면 마지막 조준 방향이 그대로 남는다.

---

## 5. 뒤로가기 — 설계

```
Android_Back 키 → AMyPlayer::OnBackPressed → UMyCanvas::ToggleExitPanel
   ├ 열기: 패널 Visible + SetGamePaused(true)
   └ 닫기: 패널 Collapsed + SetGamePaused(false)

UExitPanelWidget (WBP_ExitPanel의 부모)
   ├ ContinueButton → 패널 닫기 + 일시정지 해제
   └ ExitButton     → 일시정지 해제 + MainMenu 맵 이동
```

- 일시정지 중에도 뒤로가기를 다시 누르면 닫히도록 키 바인딩에 `bExecuteWhenPaused = true`.
- 새 C++ 클래스 1개 → **Live Coding 금지, 풀 리빌드.**

## 6. 나가기 확인 위젯 헤더 — Source/ZombieHunter1/Public/UI/ExitPanelWidget.h (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExitPanelWidget.generated.h"

class UButton;

// 뒤로가기로 여는 "메인메뉴로 나갈까요?" 패널 — WBP_ExitPanel의 부모 클래스
// => 자세한 내용은 노션 개발문서 참고
UCLASS()
class ZOMBIEHUNTER1_API UExitPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* ContinueButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* ExitButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FName MainMenuLevelName = TEXT("MainMenu");

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnExitClicked();
};
```

## 7. 나가기 확인 위젯 구현 — Source/ZombieHunter1/Private/UI/ExitPanelWidget.cpp (새 파일)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ExitPanelWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UExitPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UExitPanelWidget::OnContinueClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddUniqueDynamic(this, &UExitPanelWidget::OnExitClicked);
	}
}

void UExitPanelWidget::OnContinueClicked()
{
	UGameplayStatics::SetGamePaused(this, false);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UExitPanelWidget::OnExitClicked()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
```

## 8. 캔버스에 패널 연결 — 4곳

### 8-1. Source/ZombieHunter1/Public/UI/MyCanvas.h:17 (전방 선언)

```cpp
// Before
class UDeathPanelWidget;
```
```cpp
class UDeathPanelWidget;
class UExitPanelWidget;
```

### 8-2. Source/ZombieHunter1/Public/UI/MyCanvas.h:72 (DeathPanel 변수 아래에 추가)

```cpp
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UDeathPanelWidget* DeathPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UExitPanelWidget* ExitPanel;
```

### 8-3. Source/ZombieHunter1/Public/UI/MyCanvas.h:80 (ShowDeathPanel 선언 아래에 추가)

```cpp
    UFUNCTION(BlueprintCallable)
    void ShowDeathPanel(bool bShow);

    void ToggleExitPanel();
```

### 8-4. Source/ZombieHunter1/Private/UI/MyCanvas.cpp

include 추가 (4번째 줄 근처):

```cpp
#include "UI/DeathPanelWidget.h" //사망 패널 켜고 끄기 (SetVisibility에 완전한 타입 필요)
#include "UI/ExitPanelWidget.h"
#include "Kismet/GameplayStatics.h"
```

`NativeConstruct` 안, DeathPanel 숨기는 블록 바로 아래:

```cpp
    if (DeathPanel)
    {
        DeathPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (ExitPanel)
    {
        ExitPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
```

`ShowDeathPanel` 함수 아래에 새 함수:

```cpp
void UMyCanvas::ToggleExitPanel()
{
    if (!ExitPanel)
    {
        return;
    }

    const bool bOpen = ExitPanel->GetVisibility() == ESlateVisibility::Collapsed;
    ExitPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    UGameplayStatics::SetGamePaused(this, bOpen);
}
```

## 9. 플레이어가 뒤로가기 키를 받기 — 2곳

### 9-1. Source/ZombieHunter1/Public/Characters/MyPlayer.h:273 (마우스 핸들러 아래에 추가)

```cpp
	void OnRightMouseReleased();

	// 안드로이드 뒤로가기 → 나가기 확인 패널
	void OnBackPressed();
```

### 9-2. Source/ZombieHunter1/Private/Characters/MyPlayer.cpp — SetupPlayerInputComponent 끝(디버그 C 키 위)

```cpp
    // 안드로이드 뒤로가기: 일시정지 중에도 다시 눌러 닫을 수 있어야 한다
    FInputKeyBinding& BackBinding = PlayerInputComponent->BindKey(EKeys::Android_Back, IE_Pressed, this, &AMyPlayer::OnBackPressed);
    BackBinding.bExecuteWhenPaused = true;
```

### 9-3. Source/ZombieHunter1/Private/Characters/MyPlayer.cpp — OnRightMouseReleased 구현 아래

```cpp
void AMyPlayer::OnRightMouseReleased() { bRightMouseHeld = false; }

void AMyPlayer::OnBackPressed()
{
    if (CanvasWidget)
    {
        CanvasWidget->ToggleExitPanel();
    }
}
```

(`CanvasWidget`은 이미 `UMyCanvas*`로 쓰이고, `MyPlayer.cpp`는 이미 `UI/MyCanvas.h`를 쓰는 중이라 include 추가 불필요. 안 되면 `#include "UI/MyCanvas.h"` 추가.)

---

## 10. 에디터: WBP_ExitPanel 만들기 (풀 리빌드 후)

1. 콘텐츠 브라우저 → 우클릭 → 유저 인터페이스 → 위젯 블루프린트 → **ExitPanel**(`WBP_ExitPanel`)
2. Class Settings → Parent Class = **ExitPanelWidget**
3. 디자이너: 어두운 배경 이미지, 문구(예: "메인메뉴로 나갈까요?"), 버튼 2개
   - 이름을 **ContinueButton**, **ExitButton**으로 (BindWidget이라 이름이 틀리면 컴파일 에러)
4. `BP_Canvas` 디자이너에 `WBP_ExitPanel`을 배치하고 인스턴스 이름을 **ExitPanel**로
   - DeathPanel처럼 화면 전체를 덮는 위치/앵커로, Z 순서는 위로

## 11. 에디터: 디버그 선 / Sky Atmosphere

1. **디버그 선**: `BP_ArcherJob`, `BP_WarriorJob`, `BP_MageJob`, `BP_HealerJob` Class Defaults에서 `bDebugAttack` 체크 해제. 동료/적/플레이어 BP의 `bDebugCombat`도 확인.
2. **Sky Atmosphere**: `GamePlay` 맵 아웃라이너에서 Sky Atmosphere 액터 선택 → 삭제 → **맵 저장**.
   - 삭제 후 `git status`에서 `__ExternalActors__`의 `.uasset`이 0바이트로 남지 않았는지 확인: `git ls-files -s | findstr e69de29`
   - 같은 맵 외부 액터 파일 중 하나가 더 Sky Atmosphere 관련일 수 있다 (Sky Light, 조명 BP 등). 삭제해도 되는 건 Sky Atmosphere 액터뿐.

---

## 확인 (폰)

- [ ] 빈 화면을 터치해도 점프하지 않는다
- [ ] 오른쪽으로 공격하다가 조준을 아래로 돌리면 곧바로 아래로 나간다
- [ ] 뒤로가기 → 일시정지 + 확인 창 → 계속하기 / 나가기
- [ ] 일시정지 중 뒤로가기를 다시 누르면 창이 닫힌다
- [ ] 초록 선, 빨간 Sky Atmosphere 메시지가 없다
