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
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
    AttachToComponent(Character->GetMesh(), AttachRules, FName("hand_r"));
	// 4. 确保可见
    SetActorHiddenInGame(false);
	// 5. 清除地上的拾取提示
    if (IPickupInterface* PickupInterface = Cast<IPickupInterface>(Character))
    {
        PickupInterface->SetOverlappingItem(nullptr);
    }
}

void AWeaponActor::Drop(const FVector& DropLocation)
{
	// 1. 脱离角色
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetOwner(nullptr);
    bIsEquipped = false;
	// 2. 放到地上
	SetActorLocation(DropLocation);
	// 3. 开启物理模拟（掉在地上）
    ItemMesh->SetSimulatePhysics(true);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);  // 现在有碰撞体积
	// 4. 开启拾取检测
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 5. 给一点随机旋转，看起来自然
    SetActorRotation(FRotator(0.f, FMath::RandRange(0.f, 360.f), 90.f));  // 侧躺在地
}

void AWeaponActor::Fire(const FVector& Location, const FRotator& Rotation, float Damage)
{
	PlayFireEffects(Location, Rotation);
	if (ProjectileClass)
    {
        SpawnProjectile(Location, Rotation, Damage);
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
