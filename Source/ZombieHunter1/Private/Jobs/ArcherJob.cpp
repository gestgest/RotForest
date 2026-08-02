// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/ArcherJob.h"
#include "Projectiles/Projectile.h"

UArcherJob::UArcherJob()
{
	JobType = EJobType::Archer;

	Stats.MaxHP = 10;
	Stats.Damage = 1;
	Stats.Speed = 900;
	Stats.AttackInterval = 0.4;

	// 기본 발사체: 순수 AProjectile(메시 없는 충돌 구체). BP_Arrow로 바꾸면 화살이 보인다.
	ProjectileClass = AProjectile::StaticClass();

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_ArcherJob)에서 직접 지정한다.

	// 활은 왼손으로 들고 오른손으로 시위를 당긴다 → 왼손 슬롯을 쓴다.
	// 캐릭터의 LeftHandSocket(기본 "weapon_socket_l")이 스켈레톤에 있어야 한다.
	WeaponHand = EWeaponHand::Left;
}

 
void UArcherJob::FireArrow()
{
	// 화살은 단일 대상 발사체 (폭발 없음). 스폰은 베이스 공용 헬퍼가 담당.
	SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight);
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
	// FireArrow();  ← 삭제
}

void UArcherJob::PlayBowDraw()
{
    if (!BowDrawAnim || !OwnerCharacter) return;

    USkeletalMeshComponent* Weapon = OwnerCharacter->GetWeaponMeshComponent();
    if (!Weapon || !Weapon->GetSkeletalMeshAsset()) return;

    Weapon->PlayAnimation(BowDrawAnim, false);

    UAnimSingleNodeInstance* Single = Weapon->GetSingleNodeInstance();
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

    USkeletalMeshComponent* Weapon = OwnerCharacter->GetWeaponMeshComponent();
    if (!Weapon) return;

    if (UAnimSingleNodeInstance* Single = Weapon->GetSingleNodeInstance())
    {
        Single->SetPosition(0.0f);   // 0프레임 = 시위 풀린 상태
        Single->SetPlaying(false);
    }
}
