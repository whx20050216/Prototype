// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AbilitySet.h"
#include "AbilitySystemComponent.h"

void UAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, TArray<FGameplayAbilitySpecHandle>& OutHandles) const
{
	if (!ASC) return;

	for (const FAbilitySetEntry& Entry : Abilities)
	{
		if (!Entry.AbilityClass) return;

		// 创建Spec
		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.Level);

		// 关键：绑定InputTag到DynamicAbilityTags（这样OnAbilityInputPressed能找到）
		if (Entry.InputTag.IsValid())
		{
			Spec.DynamicAbilityTags.AddTag(Entry.InputTag);
		}

		// 授予并记录Handle
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		OutHandles.Add(Handle);
	}
}

void UAbilitySet::RemoveFromAbilitySystem(UAbilitySystemComponent* ASC, const TArray<FGameplayAbilitySpecHandle>& Handles) const
{
	if (!ASC) return;

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}
}
