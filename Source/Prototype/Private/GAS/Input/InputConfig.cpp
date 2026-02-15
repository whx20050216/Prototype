// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Input/InputConfig.h"

FGameplayTag UInputConfig::FindInputTagForAction(const UInputAction* Action) const
{
	if (!Action) return FGameplayTag();

	for (const FAbilityInputBinding& Binding : AbilityInputBindings)
	{
		if (Binding.InputAction == Action)
		{
			return Binding.InputTag;
		}
	}
	return FGameplayTag();
}

UInputAction* UInputConfig::FindInputActionForTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid()) return nullptr;

	for (const FAbilityInputBinding& Binding : AbilityInputBindings)
	{
		if (Tag == Binding.InputTag)
		{
			return Binding.InputAction.Get();
		}
	}
	return nullptr;
}
