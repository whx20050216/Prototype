// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AlexAttributeSet.h"
#include "Net/UnrealNetwork.h"

UAlexAttributeSet::UAlexAttributeSet()
{
	// 必须初始化默认值
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitStamina(100.f);
    InitMaxStamina(100.f);
}

void UAlexAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Health Replicated: %f"), GetHealth());
}

void UAlexAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Stamina Replicated: %f"), GetStamina());
}

void UAlexAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// Clamp数值
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    else if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
    }
}

void UAlexAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	// 登记每个属性："这个属性需要被监控"
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAlexAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}
