// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "WeaponActor.generated.h"

class USoundBase;
class UNiagaraSystem;
class AProjectile;
class ABaseCharacter;

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	// 基础配置
	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
	bool bAutomatic = true;			// true=M4连射, false=RPG单发

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
	float FireRate = 0.1f;          // 射击间隔（秒）

	UPROPERTY(EditDefaultsOnly, Category = "WeaponConfig")
	float ReloadTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
	TSubclassOf<UGameplayEffect> CostGE; // 弹药消耗（M4扣子弹，RPG扣火箭弹）

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
    FGameplayTag AmmoType;               // 用于UI显示和属性检查（Ammo.Bullet / Ammo.Rocket）

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
    float Damage = 20.f;                 // 基础伤害

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
    int32 MaxAmmoPerMag = 30;            // 弹匣容量（RPG=1）

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
    int32 MaxReserveAmmo = 90;			// M4=90, RPG=5

	UPROPERTY(EditDefaultsOnly, Category="WeaponConfig")
    bool bAutoReload = false;  // 敌人设为 true，玩家 false
};


UCLASS()
class PROTOTYPE_API AWeaponActor : public AItem
{
	GENERATED_BODY()
	
public:	
	AWeaponActor();

	// 换弹状态（给 UI 读取）
    UPROPERTY(BlueprintReadOnly, Category="Weapon")
    bool bIsReloading = false;
    
    // 彻底没弹（Current=0 && Reserve=0）
    UPROPERTY(BlueprintReadOnly, Category="Weapon")
    bool bIsEmpty = false;

	// 委托通知 UI
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
    UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
    FOnReloadStarted OnReloadStarted;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinished);
    UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
    FOnReloadFinished OnReloadFinished;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponEmpty);
    UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
    FOnWeaponEmpty OnWeaponEmpty;

	// 初始化弹药
	virtual void BeginPlay() override;

	// 武器数据配置（在蓝图里为M4和RPG设置不同值）
	UPROPERTY(EditDefaultsOnly, Category="Weapon", meta=(ShowOnlyInnerProperties))
	FWeaponData WeaponData;

	// 当前弹药（运行时）
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	int32 CurrentAmmo = 0;

	// 后备弹药（运行时）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	int32 ReserveAmmo = 0;

	// 换弹函数
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool CanReload() const { return ReserveAmmo > 0 && CurrentAmmo == 0; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	FTimerHandle ReloadTimer;

	// 辅助访问函数（供GA_RangeAttack调用）
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool HasAmmo() const { return CurrentAmmo > 0; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void ConsumeAmmo(int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool IsAutomatic() const { return WeaponData.bAutomatic; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	float GetFireRate() const { return WeaponData.FireRate; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	TSubclassOf<UGameplayEffect> GetCostGE() const { return WeaponData.CostGE; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	FGameplayTag GetAmmoType() const { return WeaponData.AmmoType; }
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	float GetWeaponDamage() const { return WeaponData.Damage; }

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
	virtual void Fire(const FVector& Location, const FRotator& Rotation, float OverrideDamage = -1.f);

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
