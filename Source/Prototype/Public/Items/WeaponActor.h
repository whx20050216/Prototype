// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "WeaponActor.generated.h"

class USoundBase;
class UNiagaraSystem;
class AProjectile;
class ABaseCharacter;

UCLASS()
class PROTOTYPE_API AWeaponActor : public AItem
{
	GENERATED_BODY()
	
public:	
	AWeaponActor();

	// 是否可以被拾取
	UPROPERTY(EditAnywhere, Category="Pickup")
    bool bIsPickupable = true;

	// 当前是否已被装备
    UPROPERTY(BlueprintReadOnly, Category="Pickup")
    bool bIsEquipped = false;

	// 装备到角色手上
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void EquipTo(ABaseCharacter* Character);
    
    // 丢弃到地上
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void Drop(const FVector& DropLocation);
	
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;		// 枪口位置，可拖放到枪管末端

	UFUNCTION(BlueprintCallable, Category="Weapon")
	USceneComponent* GetMuzzlePoint() const { return MuzzlePoint; }

	UPROPERTY(EditAnywhere, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, Category="Weapon|Projectile")
    TSubclassOf<AProjectile> ProjectileClass;  // 留空=纯近战（只播音效）

	UPROPERTY(EditAnywhere, Category="Weapon|VFX")
    TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;  // 枪口火焰或挥动轨迹

	UFUNCTION(BlueprintCallable, Category="Weapon")
    virtual void Fire(const FVector& Location, const FRotator& Rotation, float Damage);

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
        const FHitResult& SweepResult) override;

    // 子类可覆盖的具体实现
    UFUNCTION()
    virtual void PlayFireEffects(const FVector& Location, const FRotator& Rotation);
    
    UFUNCTION()
    virtual AProjectile* SpawnProjectile(const FVector& Location, const FRotator& Rotation, float Damage);
    
    UFUNCTION()
    AActor* GetOwnerCharacter() const;
};
