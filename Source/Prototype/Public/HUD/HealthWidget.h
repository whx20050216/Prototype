// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthWidget.generated.h"

class UAttributeComponent;
class UProgressBar;
class UTextBlock;

UCLASS(Abstract, Blueprintable, BlueprintType)	//Abstract防止直接实例化
class PROTOTYPE_API UHealthWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 初始化时绑定Component（在PlayerController或Character里调用）
    UFUNCTION(BlueprintCallable, Category="UI")
    void BindToAttributeComponent(UAttributeComponent* AttributeComp);

protected:
	UPROPERTY(meta=(BindWidget, OptionalWidget))
    UProgressBar* Bar_Health;

	UPROPERTY(meta=(BindWidget, OptionalWidget))  //OptionalWidget防止蓝图没放这个控件就报错
    UTextBlock* Text_Health;  //建议加上文本显示

	UPROPERTY(meta=(BideWidget, OptionalWidget))
	UProgressBar* Bar_Mana;

	UPROPERTY(meta=(BindWidget, OptionalWidget))
	UTextBlock* Text_Mana;

	UPROPERTY()
    UAttributeComponent* BoundAttributeComp;

	// 委托回调
    UFUNCTION()
    void OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta);

	UFUNCTION()
	void OnManaChanged(float CurrentMana, float MaxMana, float Delta);

	// 更新UI显示
    void UpdateHealthDisplay(float CurrentHealth, float MaxHealth);

	void UpdateManaDisplay(float CurrentMana, float MaxMana);

	// 解绑（防止野指针）
    virtual void NativeDestruct() override;
};
