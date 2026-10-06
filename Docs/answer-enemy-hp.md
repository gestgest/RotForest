# 답안지 — HP가 초기화 안 된 일반 좀비

## 원인
스폰 경로(`SpawnEnemy`)는 문제없다. 숨겨진 적만 깨우고 `SetHP(MaxHP)`까지 부른다.

문제는 **리쉬(`RecycleFarEnemies`)**. 플레이어에게서 `LeashDistance`(4500) 넘게 떨어졌거나 낙하한 **살아 있는** 적을
스폰 링으로 순간이동만 시키고 HP는 그대로 둔다.

재현 흐름:
1. 좀비를 몇 대 때린다 (HP 감소)
2. 그 좀비를 두고 멀리 달린다
3. 리쉬가 그 좀비를 진행 방향 앞 스폰 링에 다시 놓는다
4. 플레이어 눈엔 "새로 나온 좀비"인데 HP 바가 깎여 있음

## 0. 확인용 로그 — ZombieSlayerGameMode.cpp:202
```cpp
if (FindReachablePointInRing(playerLocation, resultLocation))
{
    UE_LOG(LogTemp, Log, TEXT("[Leash] %s HP=%d/%d"), *enemy->GetName(), enemy->GetHP(), enemy->GetMaxHP());
    enemy->TeleportForLeash(resultLocation.Location + upVector);
}
```
증상이 날 때 이 로그가 HP < MaxHP로 찍히면 확정.

## 1. 리쉬로 옮긴 적은 새 적 취급 — ZombieSlayerGameMode.cpp:200
```cpp
// Before
if (FindReachablePointInRing(playerLocation, resultLocation))
{
    enemy->TeleportForLeash(resultLocation.Location + upVector);
}
```
```cpp
// After
if (FindReachablePointInRing(playerLocation, resultLocation))
{
    enemy->TeleportForLeash(resultLocation.Location + upVector);
    enemy->SetHP(enemy->GetMaxHP());
}
```
(설계상 "쫓아오던 그 좀비"로 보이게 HP를 유지하고 싶다면 수정하지 않는 것도 선택지.)

## 2. 곁가지 — 동료가 풀에 숨은 적을 노림 — Companion.cpp:198
`FindNearestEnemy`가 `GetAllActorsOfClass`로 찾고 `GetIsDead()`만 거른다. 풀에서 대기 중인(숨겨진) 적은
안 죽은 상태라 후보에 들어간다 → 동료가 보이지 않는 적을 향해 가거나 멈춰 있을 수 있다.
```cpp
// Before
if (!E || E->GetIsDead())
```
```cpp
// After
if (!E || E->IsHidden() || E->GetIsDead())
```
