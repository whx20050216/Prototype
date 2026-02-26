// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponActor.h"
#include "Items/Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "Character/BaseCharacter.h"
#include "Interfaces/PickupInterface.h"
#include "Character/AlexCharacter.h"
#include "GAS/AlexAttributeSet.h"

AWeaponActor::AWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	if (ItemMesh)  // 从 AItem 继承来的
    {
        // 初始状态：可拾取时的碰撞设置
        ItemMesh->SetSimulatePhysics(false);
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        ItemMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 掉地上时阻挡地面
    }

	// 创建枪口点，默认在武器前方 50cm
    MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
    MuzzlePoint->SetupAttachment(ItemMesh);
    MuzzlePoint->SetRelativeLocation(FVector(50.f, 0.f, 0.f));

	// 拾取范围（继承自 AItem 的 Sphere）
    Sphere->SetSphereRadius(100.f);  // 拾取半径 1 米
}

void AWeaponActor::BeginPlay()
{
    Super::BeginPlay();

    CurrentAmmo = WeaponData.MaxAmmoPerMag;
    ReserveAmmo = WeaponData.MaxReserveAmmo;
}

void AWeaponActor::Reload()
{
	// 防止重复换弹或没弹可换
    if (bIsReloading || !CanReload()) 
    {
        if (!CanReload() && CurrentAmmo == 0 && !bIsEmpty)
        {
            bIsEmpty = true;
            OnWeaponEmpty.Broadcast();  // 通知 UI：彻底没弹了，可以丢枪
        }
        return;
    }
    
    bIsReloading = true;
    OnReloadStarted.Broadcast();  // UI 显示"换弹中..."
    
    // 延迟 0.5 秒模拟换弹动作（全自动武器也需要上膛时间）
    GetWorld()->GetTimerManager().SetTimer(ReloadTimer, [this]()
    {
        // 计算实际能换多少（最后一梭子可能不满）
        int32 Needed = WeaponData.MaxAmmoPerMag - CurrentAmmo;
        int32 ToReload = FMath::Min(Needed, ReserveAmmo);
        
        ReserveAmmo -= ToReload;
        CurrentAmmo += ToReload;
        
        // 反向同步到 GAS，UI 会自动更新 Current
        if (AAlexCharacter* Character = Cast<AAlexCharacter>(GetOwner()))
        {
            if (UAlexAttributeSet* AS = Character->GetAttributeSet())
            {
                if (WeaponData.AmmoType.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ammo.Bullet"))))
                    AS->SetAmmo_Bullet(CurrentAmmo);
                else
                    AS->SetAmmo_Rocket(CurrentAmmo);
                
                // 通知 UI 后备弹药变了（90→60）
                if (Character->OnReserveAmmoChanged.IsBound())
                    Character->OnReserveAmmoChanged.Broadcast(ReserveAmmo, WeaponData.MaxReserveAmmo);
            }
        }
        
        bIsReloading = false;
        OnReloadFinished.Broadcast();  // UI 隐藏"换弹中"
    }, WeaponData.ReloadTime, false);  // 0.5秒换弹时间
}

void AWeaponActor::ConsumeAmmo(int32 Amount)
{
	CurrentAmmo = FMath::Max(0, CurrentAmmo - Amount);
}

void AWeaponActor::EquipTo(ABaseCharacter* Character)
{
    if (!Character || !bIsPickupable) return;
	// 1. 设置拥有者
    SetOwner(Character);
    bIsEquipped = true;
	// 2. 关闭物理和拾取碰撞（装备后不再被拾取）
    ItemMesh->SetSimulatePhysics(false);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 3. 附加到右手（保持当前世界变换，避免瞬移）
	//FVector OriginalScale = ItemMesh->GetRelativeScale3D();
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
    AttachToComponent(Character->GetMesh(), AttachRules, FName("hand_r"));
	//ItemMesh->SetRelativeScale3D(OriginalScale);
	// 4. 确保可见
    SetActorHiddenInGame(false);
	// 5. 清除地上的拾取提示
    if (IPickupInterface* PickupInterface = Cast<IPickupInterface>(Character))
    {
        PickupInterface->SetOverlappingItem(nullptr);
    }
	if (AAlexCharacter* AlexChar = Cast<AAlexCharacter>(Character))
    {
        if (UAbilitySystemComponent* ASC = AlexChar->GetAbilitySystemComponent())
        {
            if (WeaponData.AmmoType.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ammo.Bullet"))))
            {
                AlexChar->GetAttributeSet()->SetAmmo_Bullet(CurrentAmmo);
            }
            else if (WeaponData.AmmoType.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ammo.Rocket"))))
            {
                AlexChar->GetAttributeSet()->SetAmmo_Rocket(CurrentAmmo);
            }
            }
		AlexChar->SetHeldWeapon(this);
    }
}

void AWeaponActor::Drop(const FVector& DropLocation)
{
	AActor* OldOwner = GetOwner();
    // 解除装备（先通知，再解除）
    if (AAlexCharacter* Character = Cast<AAlexCharacter>(OldOwner))
    {
        Character->SetHeldWeapon(nullptr);  // 触发广播，通知 UI 隐藏
    }

	// 1. 脱离角色
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetOwner(nullptr);
    bIsEquipped = false;
	// 2. 放到地上
	SetActorLocation(DropLocation);
	// 3. 开启物理模拟（掉在地上）
    ItemMesh->SetSimulatePhysics(true);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);  // 现在有碰撞体积
	FTimerHandle CollisionTimer;
    GetWorld()->GetTimerManager().SetTimer(CollisionTimer, [this]()
    {
        if (ItemMesh)
        {
            ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);  // 恢复阻挡
        }
    }, 0.5f, false);
	// 4. 开启拾取检测
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 5. 给一点随机旋转，看起来自然
    SetActorRotation(FRotator(0.f, FMath::RandRange(0.f, 360.f), 90.f));  // 侧躺在地
}

void AWeaponActor::Fire(const FVector& Location, const FRotator& Rotation, float OverrideDamage)
{
    float FinalDamage = (OverrideDamage > 0.f) ? OverrideDamage : WeaponData.Damage;

	PlayFireEffects(Location, Rotation);
	if (ProjectileClass)
    {
        SpawnProjectile(Location, Rotation, FinalDamage);
    }
}

void AWeaponActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bIsEquipped || !bIsPickupable) return;

    Super::OnSphereOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AWeaponActor::PlayFireEffects(const FVector& Location, const FRotator& Rotation)
{
	if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this, 
            FireSound, 
            Location,
            Rotation,
            1.0f,  // Volume
            1.0f,  // Pitch
            0.f,   // StartTime
            nullptr, // Attenuation (可用默认或指定)
            nullptr  // Concurrency
        );
    }

    if (MuzzleFlashEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            MuzzleFlashEffect,
            Location,
            Rotation,
            FVector(1.f),  // Scale
            true,          // AutoDestroy
            true           // AutoActivate
        );
    }
}

AProjectile* AWeaponActor::SpawnProjectile(const FVector& Location, const FRotator& Rotation, float Damage)
{
	if (!ProjectileClass) return nullptr;

	FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Instigator = Cast<APawn>(GetOwner());
    SpawnParams.Owner = this;
    
    AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
        ProjectileClass,
        Location,
        Rotation,
        SpawnParams
    );
    
    if (Projectile)
    {
        Projectile->Damage = Damage;
    }
    
    return Projectile;
}

AActor* AWeaponActor::GetOwnerCharacter() const
{
	return GetOwner();
}
