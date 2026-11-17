// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/ArcherJob.h"

#include "Jobs/SkillDataAsset.h"
#include "Characters/Enemy.h"

#include "Projectiles/Projectile.h"

#include "TimerManager.h"

UArcherJob::UArcherJob()
{
	JobType = EJobType::Archer;

	Stats.MaxHP = 10;
	Stats.Damage = 1;
	Stats.Speed = 900;
	Stats.AttackInterval = 0.4f;

	// 기본 발사체: 순수 AProjectile(메시 없는 충돌 구체). BP_Arrow로 바꾸면 화살이 보인다.
	ProjectileClass = AProjectile::StaticClass();

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_ArcherJob)에서 직접 지정한다.

    EngageRange = 1000.0f;
}

 
//화살 발사
void UArcherJob::FireArrow()
{
	// 화살은 단일 대상 발사체 (폭발 없음). 스폰은 베이스 공용 헬퍼가 담당.
	SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight);
    PlayAttackSound();
}

void UArcherJob::OnAttackMontageEnded()
{
	FireArrow();
	ResetBow();
}

// Attack()에서 즉발 발사 제거
void UArcherJob::Attack()
{
	Super::Attack();   // 몽타주 재생
	PlayBowDraw();     // 활 애니도 같이 시작
}

void UArcherJob::UseSkill()
{
    bSkillActive = true;
    SetOwnerMovementLocked(true);

    FireQuickShot();

    FTimerManager& Timers = OwnerCharacter->GetWorldTimerManager();

    Timers.SetTimer(QuickShotFireTimer, this, &UArcherJob::FireQuickShot, QuickShotFireInterval, true);
    Timers.SetTimer(QuickShotEndTimer, this, &UArcherJob::StopSkill, QuickShotMaxDuration, false);
}

void UArcherJob::StopSkill()
{
    if (!bSkillActive)
    {
        return;
    }

    if (OwnerCharacter)
    {
        FTimerManager& Timers = OwnerCharacter->GetWorldTimerManager();
        Timers.ClearTimer(QuickShotFireTimer);
        Timers.ClearTimer(QuickShotEndTimer);
    }
    SetOwnerMovementLocked(false);

    bSkillActive = false;
    StartSkillCooldown();
}

void UArcherJob::FireQuickShot()
{
    if (!OwnerCharacter || OwnerCharacter->GetIsDead())
    {
        StopSkill();
        return;
    }

    FVector Dir = OwnerCharacter->GetActorForwardVector();
    ACombatCharacter* Target = OwnerCharacter->GetSkillTarget();
    if (Target && !Target->GetIsDead())
    {
        Dir = (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
        OwnerCharacter->SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
    }

    if (AProjectile* Arrow = SpawnProjectile(ProjectileClass, Dir, ProjectileSpeed, MuzzleOffset, MuzzleHeight))
    {
        Arrow->Damage = FMath::Max(1, FMath::RoundToInt(GetDamage() * SkillData->DamageRatio));
        PlayAttackSound(); // 근데 이거 스킬 사운드 넣어야하는 거 아닌가? => 근데 어차피 지금 이 구조자체는 마음에 듬
    }
}

bool UArcherJob::WantsToUseSkill(AEnemy* Target)
{
    if (!Target || Target->GetIsDead())
    {
        return false;
    }

    // todo 이거 궁수는 속사가 평타의 상위호환 아닌까?
    return RollAISkill(OwnerCharacter->GetAttackInterval());
}




void UArcherJob::PlayBowDraw()
{
    if (!BowDrawAnim || !OwnerCharacter) return;

    USkeletalMeshComponent* WeaponComp = OwnerCharacter->GetWeaponMeshComponent();
    if (!WeaponComp || !WeaponComp->GetSkeletalMeshAsset()) return;

    WeaponComp->PlayAnimation(BowDrawAnim, false);

    UAnimSingleNodeInstance* Single = WeaponComp->GetSingleNodeInstance();
    if (!Single) return;

    // 활 시위가 몽타주 끝(=활 놓는 순간)에 딱 맞춰 다 당겨지도록 배속을 계산한다.
    // Notify 위치를 옮기거나 공속이 바뀌어도 자동으로 따라온다.
    float Rate = 1.0f;
    if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
    {
        if (UAnimMontage* Montage = Anim->GetCurrentActiveMontage())
        {
            const float Pos = Anim->Montage_GetPosition(Montage);
            const float MonRate = FMath::Max(Anim->Montage_GetPlayRate(Montage), KINDA_SMALL_NUMBER);
            const float Remaining = (Montage->GetPlayLength() - Pos) / MonRate;

            if (Remaining > KINDA_SMALL_NUMBER)
            {
                Rate = BowDrawAnim->GetPlayLength() / Remaining;
            }
        }
    }
    Single->SetPlayRate(Rate * BowDrawRateScale);
}

//시위를 원 위치로 되돌린다.
void UArcherJob::ResetBow()
{
    if (!OwnerCharacter) return;

    USkeletalMeshComponent* WeaponComp = OwnerCharacter->GetWeaponMeshComponent();
    if (!WeaponComp) return;

    if (UAnimSingleNodeInstance* Single = WeaponComp->GetSingleNodeInstance())
    {
        Single->SetPosition(0.0f);   // 0프레임 = 시위 풀린 상태
        Single->SetPlaying(false);
    }
}
