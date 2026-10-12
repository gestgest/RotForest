# 답안지 — 동료 리쉬(복귀)

> Claude 전용 기준 문서. 사용자에게 공개하지 않는다. "힌트 줘" / 추측 비교 때 이 문서를 보고 판단한다.
> 기획: 프로젝트 문서 `Docs/기획-동료리쉬.md`. 부모 클래스(AAllyCharacter) 리팩터를 하면 `if (Leader)` 분기는 `GetAnchorLocation()` 가상함수로 바뀐다 — 그때 이 문서를 고친다.

## 결정한 것
- 리쉬 판정은 `UpdateDecision()` 맨 앞. 순간이동/복귀면 거기서 `return`하고 아래 전투/추종은 타지 않는다.
- 상태에 `Returning` 추가. 복귀 중엔 `ReengageDistance` 안으로 들어올 때까지 적 무시(히스테리시스).
- `FindNearestEnemy`가 탐색 중심을 인자로 받아 "리더 주변 적" 탐색에 재사용.
- 끼임은 의사결정마다 이동량으로 판정. 복귀 중 20cm 미만 이동이 누적 3초면 순간이동.
- 새 타겟은 **리더에게서 SoftLeashDistance(1200) 안의 적만** 고른다. 이 조건이 없으면 `DetectRadius`(900, 동료 기준) 때문에 리더에게서 2100 떨어진 적까지 잡혀서 "쫓아감 → Hard 넘어 복귀 → Reengage 후 같은 적 다시"가 반복된다. Soft 안으로 제한하면 멈출 자리가 항상 1200 안이라 Hard(1800)까지 600 여유가 남고, 직업 사거리와 무관하다.
- 사용자 코드의 변수 이름: `SoftTrackingDistance`, `HardTrackingDistance` (이 문서의 `SoftLeashDistance`, `HardLeashDistance`).

| 리더와 거리(cm) | 상태 |
|---|---|
| ~1200 | 자기 주변 적 |
| 1200~1800 | 현재 타겟 유지, 새 타겟은 리더 주변 |
| 1800~ | Returning (1000 안에 오면 해제) |
| 3000~ / 끼임 3초 | 순간이동 |

## 1. 헤더 — Public/Characters/Companion.h
- [ ] 1-1. 리쉬 값 추가
- [ ] 1-2. 상태에 Returning 추가
- [ ] 1-3. FindNearestEnemy 시그니처 변경
- [ ] 1-4. 끼임 판정 변수 + TeleportToLeader 선언

### 1-1. 리쉬 값 추가 — DecisionInterval 선언 아래
```cpp
	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float SoftLeashDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float HardLeashDistance = 1800.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float ReengageDistance = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float TeleportDistance = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float StuckTeleportTime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Leash")
	float TeleportRadius = 400.0f;
```

### 1-2. 상태에 Returning 추가
```cpp
// Before
	enum class EState : uint8 { Following, Fighting };
```
```cpp
	enum class EState : uint8 { Following, Fighting, Returning };
```

### 1-3. FindNearestEnemy 시그니처 변경
```cpp
// Before
	AEnemy* FindNearestEnemy() const;
```
```cpp
	AEnemy* FindNearestEnemy(const FVector& Origin) const;
```

### 1-4. 끼임 판정 변수 + TeleportToLeader — GetEngageRange 선언 아래
```cpp
	// [리쉬]
	float StuckTime = 0.0f;
	FVector LastDecisionLocation = FVector::ZeroVector;

	void TeleportToLeader();
```

## 2. 구현 — Private/Characters/Companion.cpp
- [ ] 2-1. include 추가
- [ ] 2-2. BeginPlay에서 위치 기록
- [ ] 2-3. UpdateDecision 앞부분 교체
- [ ] 2-4. FindNearestEnemy 수정 (+ 리더 거리 필터)
- [ ] 2-5. TeleportToLeader 추가

### 2-1. include
```cpp
#include "NavigationSystem.h"
```

### 2-2. BeginPlay — TimeSinceDecision 초기화 아래
```cpp
	LastDecisionLocation = GetActorLocation();
```

### 2-3. UpdateDecision 앞부분 — `if (CurrentTarget)` 이전을 교체
```cpp
// Before
void ACompanion::UpdateDecision()
{
	CurrentTarget = FindNearestEnemy();
```
```cpp
void ACompanion::UpdateDecision()
{
	// [리쉬]
	if (Leader)
	{
		const FVector MyLoc = GetActorLocation();
		const FVector LeaderLoc = Leader->GetActorLocation();
		const float LeaderDist = FVector::Dist2D(MyLoc, LeaderLoc);

		const float Moved = FVector::Dist2D(MyLoc, LastDecisionLocation);
		LastDecisionLocation = MyLoc;
		StuckTime = (State == EState::Returning && Moved < 20.0f) ? StuckTime + DecisionInterval : 0.0f;

		if (LeaderDist > TeleportDistance || StuckTime >= StuckTeleportTime)
		{
			TeleportToLeader();
			return;
		}

		if (LeaderDist > HardLeashDistance)
		{
			State = EState::Returning;
		}

		if (State == EState::Returning)
		{
			if (LeaderDist > ReengageDistance)
			{
				CurrentTarget = nullptr;
				if (AICon)
				{
					AICon->MoveToActor(Leader, FollowDistance);
				}
				return;
			}
			State = EState::Following;
		}

		if (LeaderDist > SoftLeashDistance)
		{
			const bool bKeepTarget = IsValid(CurrentTarget) && !CurrentTarget->GetIsDead() && !CurrentTarget->IsHidden();
			if (!bKeepTarget)
			{
				CurrentTarget = FindNearestEnemy(LeaderLoc);
			}
		}
		else
		{
			CurrentTarget = FindNearestEnemy(MyLoc);
		}
	}
	else
	{
		CurrentTarget = FindNearestEnemy(GetActorLocation());
	}
```

### 2-4. FindNearestEnemy — 함수 전체 교체
리더에게서 SoftLeashDistance 밖의 적은 고르지 않는다.
```cpp
AEnemy* ACompanion::FindNearestEnemy(const FVector& Origin) const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

	AEnemy* Best = nullptr;
	float BestDistSq = DetectRadius * DetectRadius;
	const float MaxFromLeaderSq = SoftLeashDistance * SoftLeashDistance;

	for (AActor* A : Enemies)
	{
		AEnemy* E = Cast<AEnemy>(A);
		if (!E || E->IsHidden() || E->GetIsDead())
		{
			continue;
		}
		// 리더에게서 SoftLeashDistance 밖은 제외
		if (Leader && FVector::DistSquared2D(Leader->GetActorLocation(), E->GetActorLocation()) > MaxFromLeaderSq)
		{
			continue;
		}
		const float DSq = FVector::DistSquared(Origin, E->GetActorLocation());
		if (DSq < BestDistSq)
		{
			BestDistSq = DSq;
			Best = E;
		}
	}
	return Best;
}
```

### 2-5. TeleportToLeader — 파일 맨 아래
```cpp
void ACompanion::TeleportToLeader()
{
	StuckTime = 0.0f;
	CurrentTarget = nullptr;
	State = EState::Following;

	if (AICon)
	{
		AICon->StopMovement();
	}

	const FVector LeaderLoc = Leader->GetActorLocation();
	FVector Dest = LeaderLoc - Leader->GetActorForwardVector() * TeleportRadius;

	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLoc;
		if (Nav->ProjectPointToNavigation(Dest, NavLoc, FVector(200.0f, 200.0f, 500.0f)) ||
			Nav->GetRandomReachablePointInRadius(LeaderLoc, TeleportRadius, NavLoc))
		{
			Dest = NavLoc.Location;
		}
	}

	Dest.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TeleportTo(Dest, GetActorRotation());
	LastDecisionLocation = GetActorLocation();
}
```

## 빌드하고 에디터에서 설정
- [ ] 에디터 닫고 풀 리빌드 (헤더에 UPROPERTY 추가 → Live Coding 금지)
- [ ] BP_Companion 디테일 → Companion|Leash 값 확인 (1200 / 1800 / 1000 / 3000 / 3 / 400)

## 확인할 것
- [ ] 무리와 싸우다 달려가면 18m쯤에서 전투를 끊고 따라온다
- [ ] 따라오는 동료 뒤로 적이 쫓아오고, 플레이어 근처에서 다시 싸운다
- [ ] 경계(18m / 10m)에서 떨지 않는다
- [ ] 30m 넘게 벌어지면 플레이어 뒤에 나타난다
- [ ] 리더에게서 15~20m쯤 멈춰 있는 적이 있어도 쫓아감/복귀를 반복하지 않는다
- [ ] 마을 경비병(Leader 없음)은 예전과 같다

**안 될 때**
| 증상 | 원인 | 할 일 |
|---|---|---|
| 컴파일 에러: FindNearestEnemy 인자 없음 | 다른 호출부가 남음 | 전부 `FindNearestEnemy(GetActorLocation())`로 |
| 링크 에러: NavigationSystem | 모듈 누락 | Build.cs에 `"NavigationSystem"` 추가 |
| 순간이동 후 땅에 박힘/공중 | NavLoc은 바닥 높이 | 캡슐 반높이를 더했는지 |
| 경계에서 떨림 | ReengageDistance가 HardLeash보다 큼 | 1000 < 1800 확인 |
| 순간이동 반복 | 리더 근처에 NavMesh 없음 | NavMeshBoundsVolume 범위 확인 |
| 쫓아감 ↔ 복귀가 몇 초 간격으로 반복 | 리더 거리 필터 없음 (DetectRadius 900 + Soft 1200 > Hard 1800) | 2-4의 리더 거리 조건 확인 |

## 힌트 사다리 (Claude 참고용 — 한 번에 한 단계만)
1. 방향: 리쉬 판정을 `UpdateDecision()`의 어디에 두면 아래 전투 로직과 안 엉키는지 생각해보게 한다.
2. 구조: "복귀 중" 상태가 따로 필요한 이유 — 경계에서 떨림(히스테리시스)을 질문으로 유도.
3. 재료: `FindNearestEnemy`가 중심점을 받아야 하는 이유, NavMesh 투영이 필요한 이유.
4. 거의 정답: 해당 단계의 함수/조건 형태만 말로 설명. 코드 전체는 사용자가 요청할 때만 답안지 문서로.
