# RotForest

> 언리얼 엔진 5(C++ 중심)로 제작한 탑다운 서바이벌 아케이드 게임입니다.
> 절차적으로 무한히 생성되는 숲을 탐험하며, 직업을 골라 적들을 잡고 돈을 모아
> 동료를 늘리고 무기를 강화하는 한 판(run) 단위의 게임입니다.

| 항목 | 내용 |
|---|---|
| 장르 | 탑다운 트윈스틱 / 서바이벌 아케이드 |
| 엔진 | Unreal Engine 5.4 (C++ 로직 + Blueprint 데이터·연출) |
| 플랫폼 | PC(키보드·마우스/패드), Android(가상 조이스틱) |
| 개발 기간 | 2025.12.15 ~ 2026.02.14 / 2026.06.15 ~ 진행 중 |
| 인원 | 1인 개발 |
| 영상 | [플레이 영상 (YouTube)](https://youtu.be/d2GGSKTJa9c) |

---
# 에셋 목록
- [ElfArden](https://www.fab.com/listings/53b68688-f8c0-4bc3-8612-7dce8df63b87)
- [SKnight_modular](https://www.fab.com/listings/fc3a309a-a3eb-46de-bebe-dcb40dc31e48)
- [Necropolis](https://www.fab.com/listings/b3d214c2-50fa-4a0e-a780-bee56c1baf8f)
- [Assassin](https://www.fab.com/listings/c12ff2cb-2548-4b5a-bca6-7f52f7a85ce6)
- [VFX_Magic](https://www.fab.com/listings/da3e48f2-d703-4233-b667-d3f57a4c787a)


---
# 문서
- [게임 플레이](Docs/Gameplay.md)


### 전투 시스템
![bandicam 2025-12-14 21-58-13-948](https://github.com/user-attachments/assets/6d7ee84f-4018-4288-8023-b7dcdfd99186)
- 공격에 피격될 때 마다 HP가 1씩 줄어듭니다. 체력 게이지를 통해 실시간으로 생존 상태를 확인할 수 있습니다.
- 좀비들은 플레이어와 마주치면 따라갑니다.


### 코인 수집
![bandicam 2025-12-14 21-58-13-948 (1)](https://github.com/user-attachments/assets/0637a540-b307-41fb-8c95-13b5a1e2f602)
<img width="1320" height="777" alt="image" src="https://github.com/user-attachments/assets/dde600bb-6d93-46c2-bddb-ce46079f8308" />
- 맵에 배치된 코인을 수집하여 점수를 획득합니다.

### GameOver
![bandicam 2025-12-14 21-58-13-948 (2)](https://github.com/user-attachments/assets/d29fe1a3-ba16-4fca-80b8-d2122d538f8b)
- HP가 0이 되면 플레이어는 쓰러집니다.

<img width="773" height="365" alt="image" src="https://github.com/user-attachments/assets/37c655aa-f767-4b2a-8e35-d2fc87c951bc" /><br>
- 이후 버튼을 누르면 HP가 가득 채워진 채 부활합니다.


### 적, 코인 생성
 ![bandicam 2026-02-13 00-58-35-815_success](https://github.com/user-attachments/assets/f7958179-a355-402e-aa4b-c12fcf40be96)<br>
오브젝트 pooling 알고리즘을 활용해서 적과 코인을 주변에 생성합니다.

### 공격
![bandicam 2026-01-09 02-46-22-305](https://github.com/user-attachments/assets/404c3035-8a47-4845-8f66-2f201bcc42a2)


