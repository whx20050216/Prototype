// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "FormWheelWidget.generated.h"

UCLASS()
class PROTOTYPE_API UFormWheelWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 初始化时传入可用形态（已解锁的）
	UFUNCTION(BlueprintCallable, Category="Form Wheel")
	void SetupWheel(const TArray<FGameplayTag>& AvailableForms);

	// 每帧更新鼠标位置，计算当前指向的形态
	UFUNCTION(BlueprintCallable, Category="Form Wheel")
	void UpdateMousePosition(const FVector2D& MouseScreenPos, const FVector2D& ScreenCenter);

	// 获取当前鼠标指向的形态（松开R时使用）
	UFUNCTION(BlueprintPure, Category="Form Wheel")
	FGameplayTag GetSelectedFormTag() const { return CurrentSelectedForm; }

	// 检查形态是否已解锁（UI灰显用）
	UFUNCTION(BlueprintPure, Category="Form Wheel")
	bool IsFormAvailable(FGameplayTag FormTag) const;

protected:
	// 固定形态顺序（顺时针）：拳(上) -> 爪(右上) -> 鞭(右下) -> 锤(左下) -> 刃(左上)
	// 角度定义：0度=右，90度=上，180度=左，270度=下
	const TArray<FGameplayTag> FormOrder = {
		FGameplayTag::RequestGameplayTag(FName("Form.Claw")),     // 0: 爪 (342°~54°)
        FGameplayTag::RequestGameplayTag(FName("Form.Default")),  // 1: 拳 (54°~126°)
        FGameplayTag::RequestGameplayTag(FName("Form.Blade")),    // 2: 刃 (126°~198°)
        FGameplayTag::RequestGameplayTag(FName("Form.Hammer")),   // 3: 锤 (198°~270°)
        FGameplayTag::RequestGameplayTag(FName("Form.Whip"))      // 4: 鞭 (270°~342°)
	};

	// 当前鼠标指向的形态
	UPROPERTY(BlueprintReadWrite, Category="Form Wheel")
	FGameplayTag CurrentSelectedForm;

	// 已解锁的形态集合（从AlexCharacter传入）
	UPROPERTY()
	TSet<FGameplayTag> AvailableFormSet;

	// 角度转形态的核心逻辑
	FGameplayTag GetFormByAngle(float AngleDegrees) const;
};
