# 0.1.1 — 핫픽스 (0.1.0 기반)

표시 이름 `0.1.1-beta` / Store Version `2`
목표: 폰에서 플레이할 때 바로 걸리는 문제만 잡는다. 기능 추가는 0.2.0.

원칙
- 버그 하나당 원인 조사 1~2시간. 넘기면 일단 넘기고 다음 항목으로.
- 새 C++ 클래스를 추가했다면 Live Coding 금지 → 풀 리빌드.

---

## 범위

| # | 종류 | 항목 | 상태 |
|---|---|---|---|
| 1 | 버그 | 화면 터치하면 점프함 | [ ] |
| 2 | 버그 | 오른쪽을 바라보다 아래로 공격하면 오른쪽으로 공격함 | [ ] |
| 3 | 편의 | 안드로이드 뒤로가기 → 일시정지 + "메인메뉴로 나갈까요?" 확인 창 | [ ] |
| 4 | 정리 | 직업 BP의 `bDebugAttack` 끄기 (초록 디버그 선) | [ ] |
| 5 | 정리 | GamePlay 맵의 Sky Atmosphere 액터 삭제 (모바일 경고 메시지) | [ ] |

---

## 항목별 메모

### 1. 터치하면 점프
- 조사 전. 터치 입력이 어디로 매핑되는지부터 본다.
- 확인할 곳: 입력 설정(터치 → 마우스/점프 키), 터치 인터페이스(가상 조이스틱) 설정, 점프 입력 바인딩.

### 2. 공격 방향
- 조사 전. 공격 방향이 "바라보는 방향"과 "입력 방향" 중 무엇을 쓰는지 본다.
- 확인할 곳: 직업별 `Attack` 함수의 조준 방향(`AimDir`) 계산, 오른쪽 조이스틱 입력 처리.

### 3. 뒤로가기
- 안드로이드 뒤로가기는 `Android_Back` 키 입력으로 들어온다.
- 흐름: 키 입력 → `Set Game Paused(true)` → 확인 위젯 표시
  - 계속하기: 위젯 제거 + 일시정지 해제
  - 나가기: 일시정지 해제 → 메인메뉴 이동 (`DeathPanelWidget::OnMainMenuClicked` 재사용 검토)
- 메인메뉴 레벨에서는 뒤로가기를 눌러도 앱이 꺼지지 않게 할지 결정한다.

### 4. 디버그 선
- 직업 BP(BP_ArcherJob, BP_WarriorJob 등) Class Defaults에서 `bDebugAttack` 체크 해제.
- `bDebugCombat`(CombatCharacter), `bDrawDebug`(Projectile)도 함께 확인.

### 5. Sky Atmosphere
- 모바일은 Sky Atmosphere를 자동으로 그리지 않는다. 카메라가 내려다보는 구도라 하늘이 안 보이므로 액터 삭제.
- 삭제 후 저장 확인. (삭제한 액터의 `__ExternalActors__` 파일이 0바이트로 남지 않았는지 `git status`로 확인)

---

## 릴리즈 체크리스트

- [ ] 1~5 완료
- [ ] 프로젝트 세팅 → 플랫폼 → Android: Version Display Name `0.1.1-beta`, Store Version `2`
- [ ] 코드 변경이 있으면 풀 리빌드 (에디터 종료 후 빌드)
- [ ] 패키징 (Android ASTC)
- [ ] 폰에서 확인
  - [ ] MainMenu → GameReady → GamePlay 이동
  - [ ] 터치해도 점프 안 함
  - [ ] 4방향 공격 방향 정상
  - [ ] 뒤로가기 → 확인 창 → 계속하기 / 나가기
  - [ ] 초록 선, 빨간 Sky Atmosphere 메시지 없음
- [ ] 0바이트 `.uasset` 확인: `git ls-files -s | findstr e69de29`
- [ ] 커밋 (`fix ...`) 후 `git tag v0.1.1`
- [ ] APK는 커밋하지 않는다 (`.gitignore` 확인)

---

## 0.2.0 으로 미루는 것

- 보스 심화
- 설정 버튼 (무엇을 설정할지부터 결정)
- 자동조준
- rotforest 아이콘
- 모바일 최적화 (텍스처 해상도, Device Profiles, 조명 방향 결정)
