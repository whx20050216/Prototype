// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputConfig.generated.h"

class UInputAction;

// 单个输入绑定配置
USTRUCT()
struct FAbilityInputBinding {
    GENERATED_BODY()
    
    // 输入动作资产（编辑器里设的 IA_Attack, IA_Dash 等）
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UInputAction> InputAction;
    
    // 对应的 GameplayTag（如 Ability.Attack, Ability.Dash）
    // Meta 里的 Categories 限制只能在 "Ability" 命名空间下选
    UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Ability"))
    FGameplayTag InputTag;
};

UCLASS()
class PROTOTYPE_API UInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 所有能力输入绑定列表（meta作用：数组元素标题显示为绑定的输入Tag）
    UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "InputTag"))
    TArray<FAbilityInputBinding> AbilityInputBindings;

	// 辅助函数：根据 InputAction 找对应的 Tag
	FGameplayTag FindInputTagForAction(const UInputAction* Action) const;

	// 辅助函数：根据 Tag 找 InputAction（UI显示用）
    UInputAction* FindInputActionForTag(const FGameplayTag& Tag) const;
};
