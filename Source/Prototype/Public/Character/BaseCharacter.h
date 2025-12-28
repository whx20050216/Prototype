// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/TargetableInterface.h"
#include "BaseCharacter.generated.h"

class UAttributeComponent;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterMontageEnded, UAnimMontage*, Montage, bool, bInterrupted);

// 通用的动画配置结构体
USTRUCT(BlueprintType)
struct FCharacterAnimation
{
	GENERATED_BODY()

	// 动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	UAnimMontage* Montage = nullptr;

	// 默认播放段落（可留空，播放整个蒙太奇）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	FName SectionName = NAME_None;

	// 播放速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation", meta=(ClampMin = 0.1))
	float PlayRate = 1.0f;

	// 是否循环
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	bool bLoop = false;

	// 播放权重（用于随机选择）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation", meta=(ClampMin = 0.0))
	float PlayWeight = 1.0f;
};


UCLASS()
class PROTOTYPE_API ABaseCharacter : public ACharacter, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	virtual FVector GetTargetLocation_Implementation() const override;
    virtual float GetLockPriority_Implementation() const override;
    virtual void OnLocked_Implementation(AController* Locker) override;
    virtual void OnUnlocked_Implementation(AController* Unlocker) override;

	// 子类可以Override的配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
    float LockPriority = 10.f;  // 默认10，Boss可以Override成100
	
	// 播放蒙太奇（子类可重写，支持自定义逻辑）
	virtual void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName, float PlayRate = 1.0f);

	// 便捷的动画播放接口（自动处理空检查）
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayAnimation(UAnimMontage* Montage, FName SectionName = NAME_None, float PlayRate = 1.0f);

	// 检查是否正在播放蒙太奇（带段落名检查）
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool IsPlayingMontage(UAnimMontage* Montage, FName SectionName = NAME_None) const;

	// 停止所有蒙太奇
	UFUNCTION(BlueprintCallable, Category="Animation")
	void StopAllMontages(float BlendOutTime = 0.25f);

	// 动画结束委托
	UPROPERTY(BlueprintAssignable, Category="Animation")
	FOnCharacterMontageEnded OnMontageEnded;

protected:
	virtual void BeginPlay() override;

	// UFUNCTION：绑定到动画实例的委托
	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
