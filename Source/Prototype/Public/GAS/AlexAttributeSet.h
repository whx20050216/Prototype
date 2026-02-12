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
    UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Health)
    
    UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, MaxHealth)
    
    // 耐力值
    UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Stamina)
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, Stamina)
    
    UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStamina)
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS(UAlexAttributeSet, MaxStamina)

    // 回调函数（用于调试）
    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;
    
    UFUNCTION()
    void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;
    
    // 服务器端验证，防止数据异常
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};