// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AlexAttributeSet.generated.h"

// 宏定义，必须的
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROTOTYPE_API UAlexAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
    UAlexAttributeSet();

    // 生命值
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Health)
    
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, MaxHealth)
    
    // 耐力值
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Mana)
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Mana)
    
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxMana)
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, MaxMana)

    // 冲刺次数
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_DashCharges)
    FGameplayAttributeData DashCharges;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, DashCharges)

	// 最大冲刺次数（升级时修改这个）
    UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxDashCharges)
    FGameplayAttributeData MaxDashCharges;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, MaxDashCharges)

    // 弹药
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_AmmoBullet)
	FGameplayAttributeData Ammo_Bullet;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Ammo_Bullet)

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_AmmoRocket)
	FGameplayAttributeData Ammo_Rocket;
	ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Ammo_Rocket)

    // 委托（用于UI绑定）
    FSimpleDelegate OnHealthChanged;
    FSimpleDelegate OnManaChanged;
    FSimpleDelegate OnAmmoBulletChanged;
    FSimpleDelegate OnAmmoRocketChanged;


protected:
    // 回调函数（用于调试）
    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_Mana(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;

	// GAS 回调：值变化时触发
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UFUNCTION()
    void OnRep_DashCharges(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_MaxDashCharges(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    void OnRep_AmmoBullet(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    void OnRep_AmmoRocket(const FGameplayAttributeData& OldValue) const;
    
    // 服务器端验证，防止数据异常
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 添加 PostGameplayEffectExecute 声明（用于同步到 Weapon）
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};