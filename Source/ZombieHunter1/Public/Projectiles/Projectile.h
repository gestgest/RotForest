// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USoundBase;
class UNiagaraSystem;

/**
 * 화살/투사체 액터.
 * 직선으로 날아가다가 적(AEnemy)과 겹치면 피해 + 넉백을 주고 사라진다.
 * 궁수/마법사 직업이 스폰해서 사용한다. 메시는 BP 서브클래스에서 지정.
 */
UCLASS()
class ZOMBIEHUNTER1_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	/** 적중 시 가하는 피해량. 스폰한 직업이 설정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	int32 Damage = 1;

	/** 적중 시 적을 밀어내는 힘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float KnockbackForce = 300.0f;

	/** >0이면 적중 지점 주변 이 반경(cm) 안의 모든 적에게 피해(범위 폭발). 0이면 단일 대상. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ExplosionRadius = 0.0f;

	/** 디버깅 범위 표시 => 어차피 job에서 설정해준다. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Debug")
	bool bDrawDebug = false;

	/** 발사 속도(cm/s)를 설정한다. 직업이 스폰 직후 호출. */
	void SetInitialSpeed(float Speed);

	/** 발사 후 이 시간(초)이 지나면 풀로 반환된다 (빗나간 화살 정리). 구 InitialLifeSpan 대체. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LifeSeconds = 5.0f;

	/** 풀에서 꺼내질 때(첫 스폰 포함) 풀 서브시스템이 호출 — 충돌/이동/수명 타이머를 켠다. */
	void ActivateFromPool();

	/** 적중·수명만료 시 자신을 비활성화하고 풀로 반환한다. 풀이 없으면 Destroy로 폴백. */
	void ReturnToPool();


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	// 변수
	// 발사체 시각 메시 (BP에서 메시 지정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USceneComponent* VisualRoot;

	// VisualRoot 아래에 BP가 붙인 시각 컴포넌트들. BeginPlay에서 한 번 모아두고
	// 풀 활성/비활성 때 순회한다. (Activate/Deactivate는 자식으로 전파되지 않는다)
	UPROPERTY()
	TArray<USceneComponent*> CachedVisuals;


	// 적중 시 터뜨릴 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	UNiagaraSystem* HitEffect = nullptr;     // ← 추가


	// 적중 시 재생할 사운드 (선택)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	USoundBase* HitSound = nullptr;







	/** 풀에서 쉬는 동안 보이지 않게 + 충돌/이동/틱 정지. */
	void DeactivateForPool();

	/** LifeSeconds 경과 콜백 */
	void OnLifeExpired();

	/** 수명 타이머 핸들 */
	FTimerHandle LifeTimerHandle;

	/** 비행 중인지 — 풀 반환 후 같은 프레임에 남은 오버랩 이벤트가 중복 타격하는 것을 막는다. */
	bool bInFlight = false;

	/** 충돌 구체 (루트) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* CollisionSphere;


	/** 직선 비행 이동 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	/** 디버그 사거리 라인용: 발사 시점의 위치/방향 기록 */
	FVector SpawnLocation = FVector::ZeroVector;
	FVector SpawnForward = FVector::ForwardVector;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 적 하나에게 피해 + 폭발/발사 중심에서 바깥 방향으로 넉백 */
	void ApplyHit(class AEnemy* Enemy);
};
