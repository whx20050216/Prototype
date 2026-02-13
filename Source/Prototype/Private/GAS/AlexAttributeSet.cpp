// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AlexAttributeSet.h"
#include "Net/UnrealNetwork.h"

UAlexAttributeSet::UAlexAttributeSet()
{
	// 必须初始化默认值
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(100.f);
    InitMana(100.f);
    InitDashCharges(2.f);
    InitMaxDashCharges(2.f);
}

void UAlexAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Health Replicated: %f"), GetHealth());
}

void UAlexAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("Mana Replicated: %f"), GetMana());
}

void UAlexAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
}

void UAlexAttributeSet::OnRep_DashCharges(const FGameplayAttributeData& OldValue) const
{
	UE_LOG(LogTemp, Log, TEXT("DashCharges Change: %f -> %f"), OldValue.GetCurrentValue(), GetDashCharges());
}

void UAlexAttributeSet::OnRep_MaxDashCharges(const FGameplayAttributeData& OldValue) const
{
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
}
