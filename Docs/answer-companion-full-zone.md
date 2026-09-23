# 답안지 — 동료가 꽉 찼을 때 소환 발판이 돈을 먹는 버그

## 진단 (기준: `gestgest/rotforest` 2184520)

동료 3명이 꽉 찬 상태에서 소환 발판에 서면 5원이 빠지고 아무 일도 일어나지 않는다.

```
AGaugeZone::Tick
 └ TryFillOnce  ← 여기서 돈이 빠진다 (1원 × 5번)
 └ CompleteZone ← FilledAmount = 0 리셋
    └ HandleZoneFilled → RecruitCompanion → CanRecruit가 false → 그냥 return
```

해결: 소환 발판에서 `TryFillOnce`를 오버라이드해 **돈을 받기 전에** `CanRecruit`로 막는다. 섭외 가능하면 결제는 부모(`AMoneyPadZone`)에 그대로 넘긴다.

작업 순서: 1 → 2 → 3 → 풀 리빌드(헤더에 함수 추가) → PIE 확인

---

## 1. CanRecruit를 public으로 — Source/ZombieHunter1/Public/Characters/PartyComponent.h:52

`private`의 `// [섭외]` 아래에서 빼서 `public`의 `RecruitCompanion` 아래로 옮긴다.

```cpp
	// 동료 섭외 — 소유자 옆에 동료를 스폰해 따라다니며 싸우게 한다. 동료 소환 발판(ACompanionSpawnZone)이 호출.
	void RecruitCompanion(TSubclassOf<UJobComponent> JobComponent);

	// 섭외 가능한 상태인지 검사하고, 죽은 동료를 명단에서 정리한다.
	bool CanRecruit(UWorld* World);
```

---

## 2. 소환 발판 헤더 — Source/ZombieHunter1/Public/Zones/CompanionSpawnZone.h:22

`protected:` 아래, `HandleZoneFilled` 위에 추가.

```cpp
protected:
	// 파티가 꽉 차면 돈을 받지 않는다
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;

	// 게이지 완성 → 동료 소환 (C키 섭외와 동일 로직 재사용)
	virtual void HandleZoneFilled(AMyPlayer* Player) override;
```

---

## 3. 소환 발판 구현 — Source/ZombieHunter1/Private/Zones/CompanionSpawnZone.cpp:17

`HandleZoneFilled` 아래에 추가.

```cpp
bool ACompanionSpawnZone::TryFillOnce(AMyPlayer* Player, int32& OutAmount)
{
	UPartyComponent* Party = Player ? Player->GetParty() : nullptr;
	if (!Party || !Party->CanRecruit(GetWorld()))
	{
		return false;
	}

	return Super::TryFillOnce(Player, OutAmount);
}
```

- `OutAmount`를 직접 채우지 않는다. 돈 빼기·돈 부족 사운드·남은 양만큼만 결제는 전부 `Super`가 한다. 직접 `1`을 넣으면 돈 없이 게이지가 찬다.
- `World` null 체크는 `CanRecruit` 안에 이미 있다.
- 판단을 `CanRecruit` 하나로 하므로, 발판이 돈을 받는 조건과 섭외가 성공하는 조건이 항상 같다.

---

## 확인

1. 동료 3명 섭외 → 소환 발판에 선다 → 돈이 줄지 않고 게이지도 안 오름
2. 동료가 1~2명일 때는 예전처럼 돈이 빠지고 소환됨
3. 동료 1명이 죽은 **직후(시체 남은 3초 안)** 선다 → 돈이 안 빠져야 정상
4. 시체가 사라진 뒤 다시 선다 → 소환됨
5. `BP_MyPlayer`의 Party 컴포넌트에서 `CompanionClass`를 비운 채 서 보면 주황 디버그 메시지가 0.15초마다 쌓인다 — 설정 실수를 알리는 용도라 그대로 둬도 되지만, 거슬리면 메시지 키를 `-1` 대신 고정값으로 바꿔 한 줄로 덮어쓰게 한다

### 이번 범위 밖 (결정만 해두기)

- 꽉 찬 상태로 올라왔을 때 "동료가 가득 찼습니다." 알림 — `OnPlayerEntered` 오버라이드로 한 번만 띄우면 된다
- 게이지를 반쯤 채운 뒤 파티가 꽉 차는 경우 이미 낸 돈을 돌려줄지 — 지금은 게이지에 그대로 남는다
- 발판에 "지금 못 씀" 시각 표시가 필요해지면, 결제 없이 물어볼 수 있는 `CanFill` 같은 질문용 함수를 `AGaugeZone`에 따로 두는 구조로 바꾼다
