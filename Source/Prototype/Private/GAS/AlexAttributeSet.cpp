// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AlexAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Character/AlexCharacter.h"
#include "GameplayEffectExtension.h"
#include "Items/WeaponActor.h"

UAlexAttributeSet::UAlexAttributeSet()
{
	// 必须初始化默认值
    InitHealth(1000.f);
    InitMaxHealth(1000.f);
    InitMana(100.f);
    InitMaxMana(100.f);
    InitDashCharges(2.f);
    InitMaxDashCharges(2.f);
    InitAmmo_Bullet(30.f);
    InitAmmo_Rocket(1.f);
}

void UAlexAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Health Replicated: %f"), GetHealth());
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAlexAttributeSet, Health, OldValue);
    OnHealthChanged.ExecuteIfBound();
}

void UAlexAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Mana Replicated: %f"), GetMana());
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlexAttributeSet, Mana, OldValue);
    OnManaChanged.ExecuteIfBound();
}

void UAlexAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        OnHealthChanged.ExecuteIfBound();
    }
    else if (Attribute == GetManaAttribute())
    {
        OnManaChanged.ExecuteIfBound();
    }
}

void UAlexAttributeSet::OnRep_DashCharges(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("DashCharges Change: %f -> %f"), OldValue.GetCurrentValue(), GetDashCharges());
}

void UAlexAttributeSet::OnRep_MaxDashCharges(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::OnRep_AmmoBullet(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlexAttributeSet, Ammo_Bullet, OldValue);
	OnAmmoBulletChanged.ExecuteIfBound();
}

void UAlexAttributeSet::OnRep_AmmoRocket(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAlexAttributeSet, Ammo_Rocket, OldValue);
	OnAmmoRocketChanged.ExecuteIfBound();
}

void UAlexAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// Clamp数值
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
    }
    else if (Attribute == GetDashChargesAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxDashCharges());
    }
	else if (Attribute == GetAmmo_BulletAttribute())  // 本地预测触发
    {
        OnAmmoBulletChanged.ExecuteIfBound();
    }
    else if (Attribute == GetAmmo_RocketAttribute())
    {
        OnAmmoRocketChanged.ExecuteIfBound();
    }
}

void UAlexAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	// 登记每个属性："这个属性需要被监控"
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, DashCharges, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, MaxDashCharges, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Ammo_Bullet, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Ammo_Rocket, COND_None, REPNOTIFY_Always);
}

void UAlexAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

	// 获取 Owner（AlexCharacter）
    if (AAlexCharacter* Character = Cast<AAlexCharacter>(GetOwningActor()))
    {
        if (AWeaponActor* Weapon = Character->GetHeldWeapon())
        {
            // 子弹变化 -> 同步到 Weapon.CurrentAmmo
            if (Data.EvaluatedData.Attribute == GetAmmo_BulletAttribute())
            {
                Weapon->CurrentAmmo = GetAmmo_Bullet();
            }
            // 火箭弹变化
            else if (Data.EvaluatedData.Attribute == GetAmmo_RocketAttribute())
            {
                Weapon->CurrentAmmo = GetAmmo_Rocket();  // 注意：Weapon用一个字段存当前弹药即可，通过AmmoType区分
            }
        }
    }
}
