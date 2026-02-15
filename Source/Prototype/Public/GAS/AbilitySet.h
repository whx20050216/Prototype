// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilitySet.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
struct FGameplayAbilitySpecHandle;

USTRUCT()
struct FAbilitySetEntry
{
	GENERATED_BODY()

	// 授予什么技能
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityClass;

	// 绑定什么输入（Meta 里的 Categories 限制只能在 "Ability" 命名空间下选）
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Ability"))
	FGameplayTag InputTag;

	// 技能等级（用于伤害计算等）
	UPROPERTY(EditDefaultsOnly)
	int32 Level = 1;
};

UCLASS()
class PROTOTYPE_API UAbilitySet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 该形态包含的所有技能（meta作用：数组元素标题显示为绑定的输入Tag）
	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "InputTag"))
	TArray<FAbilitySetEntry> Abilities;

	// 授予技能给ASC，返回Handles用于后续移除
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, TArray<FGameplayAbilitySpecHandle>& OutHandles) const;

	// 根据Handles移除技能
	void RemoveFromAbilitySystem(UAbilitySystemComponent* ASC, const TArray<FGameplayAbilitySpecHandle>& Handles) const;
};
