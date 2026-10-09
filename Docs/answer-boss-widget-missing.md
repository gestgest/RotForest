# 답안지 — 보스 Widget이 안 뜸

## 진단 요약
보스 접근 시 흐름: `ABoss::UpdateEncounter` → `UMyCanvas::StartBossEncounter` → `BossStatusWidget->StartEncounter` → `SetVisibility(HitTestInvisible)`

이 중 어디서 끊기는지 조사한 결과 (코드는 안 건드림, 빌드/실행도 안 함. 파일과 로그만 확인):

- **원인 1 (주범): 이름 불일치 → 포인터가 nullptr**
  - `MyCanvas.h:52` 의 멤버 이름은 `BossStatusWidget` (`BindWidgetOptional`)
  - `BP_Canvas.uasset` 안의 위젯 인스턴스 이름은 아직 `BossHPBar` (`PlayerHPBar`는 바꿨는데 이건 못 바꿈)
  - `BindWidgetOptional`은 이름이 안 맞아도 컴파일 에러가 안 난다 → nullptr
  - `StartBossEncounter` / `EndBossEncounter` 가 `if (BossStatusWidget)` 로 감싸져 있어서 **조용히 아무 일도 안 함** (경고 로그도 없음)
- **원인 2 (잠재 버그): 캔버스가 아직 없을 때 전투 상태가 먼저 켜짐**
  - `Boss.cpp:184` 에서 `bInCombat = true` 를 먼저 하고, 그 뒤 `Canvas`가 nullptr이면 return
  - 다음 틱부터는 `bShouldCombat == bInCombat` 이라 **다시는 시도하지 않음**
  - 보스가 플레이어 캔버스 연결(`SetCanvasWidget`)보다 먼저 범위 안에 들어오면 한 번 놓치고 끝

원인 1만 고쳐도 대부분 뜬다. 원인 2는 타이밍에 따라 가끔 안 뜨는 증상으로 나타난다.

## 1. 위젯 이름 맞추기 — 에디터 (코드 수정 없음)
`Content/UI/BP/Panel/BP_Canvas` → Designer 계층 구조에서 `WBP_BossStatusWidget` 인스턴스 선택
- 이름: `BossHPBar` → **`BossStatusWidget`**
- Is Variable 체크 확인
- 컴파일 후 저장

(반대로 C++ 쪽을 `BossHPBar` 로 되돌려도 되지만, 클래스 이름이 BossStatusWidget 이니 BP 이름을 바꾸는 쪽을 권장)

## 2. 캔버스가 없으면 전투 상태를 켜지 않기 — Boss.cpp:180~187
```cpp
// Before
    if (bShouldCombat == bInCombat)
    {
        return;
    }
    bInCombat = bShouldCombat;

    UMyCanvas* Canvas = GetPlayerCanvas();
    if (!Canvas)
    {
        return;
    }
```
```cpp
// After
    if (bShouldCombat == bInCombat)
    {
        return;
    }

    // 캔버스가 없으면 상태를 바꾸지 않는다 => 다음 틱에 다시 시도
    UMyCanvas* Canvas = GetPlayerCanvas();
    if (!Canvas)
    {
        return;
    }
    bInCombat = bShouldCombat;
```

## 확인 방법
- [ ] BP_Canvas의 위젯 이름이 `BossStatusWidget` 인지 (Designer 계층 구조)
- [ ] 보스 1500 안으로 접근 → 하단 바가 뜨고 0 → 100% 차오름
- [ ] 때리면 바 감소 + 누적 데미지 숫자
- [ ] 3000 밖으로 나가면 사라짐

## 힌트 사다리 (사용자가 "힌트 줘" 하면 한 단계씩)
1. 보스 쪽 `Tick`은 잘 돌고 있다. 끊기는 건 "캔버스 → 위젯" 사이다.
2. `MyCanvas.cpp` 의 `StartBossEncounter` 안, `if (...)` 조건을 보자. 이 조건이 거짓이면?
3. `BindWidgetOptional` 은 어떤 조건에서 nullptr 이 되는가? (BP 쪽에서 무엇을 맞춰야 하나)
4. `BP_Canvas` Designer에서 보스 위젯 인스턴스 이름과 `MyCanvas.h:52` 의 변수 이름을 비교해 보자.
