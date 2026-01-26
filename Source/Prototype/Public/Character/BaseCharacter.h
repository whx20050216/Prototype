// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/TargetableInterface.h"
#include "BaseCharacter.generated.h"

class UAttributeComponent;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterMontageEnded, UAnimMontage*, Montage, bool, bInterrupted);

UENUM(BlueprintType)
enum class ESectionPlayMode : uint8
{
	Single UMETA(DisplayName = "Single Play"),
	Random UMETA(DisplayName = "Random Play"),
	Sequential UMETA(DisplayName = "Sequential Play")
};

// 通用的动画配置结构体
USTRUCT(BlueprintType)
struct FCharacterAnimation
{
	GENERATED_BODY()

	// 动画蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	UAnimMontage* Montage = nullptr;

	// 单个Section
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	FName SectionName = NAME_None;

	// Section序列
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",meta=(EditCondition="bUseSectionSequence"))
	TArray<FName> SectionSequence;

	// 是否使用Section序列
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bUseSectionSequence = false;

	// 播放模式
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation", meta=(EditCondition="bUseSectionSequence"))
	ESectionPlayMode PlayMode = ESectionPlayMode::Sequential;

	// 播放速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation", meta=(ClampMin = 0.1))
	float PlayRate = 1.0f;

	// 是否循环
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	bool bLoop = false;
};

UCLASS()
class PROTOTYPE_API ABaseCharacter : public ACharacter, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	// ITargetableInterface实现
	virtual FVector GetTargetLocation_Implementation() const override;
    virtual float GetLockPriority_Implementation() const override;
    virtual void OnLocked_Implementation(AController* Locker) override;
    virtual void OnUnlocked_Implementation(AController* Unlocker) override;

	// AttackEnd
	UFUNCTION(BlueprintCallable, Category="Animation")
	virtual void AttackEnd();

	// 子类可以Override的配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
    float LockPriority = 10.f;  // 默认10，Boss可以Override成100
	
	// 播放蒙太奇（子类可重写，支持自定义逻辑）
	virtual void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName, float PlayRate = 1.0f);

	// 随机播放蒙太奇Section
	void PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionSequence, float PlayRate = 1.0f);

	// 顺序播放蒙太奇Section
	void PlaySequentialMontageSections(UAnimMontage* Montage, float PlayRate = 1.0f);

	// 获取下一个Section
	int32 GetNextSectionIndex() const;

	// 单个section动画播放接口（自动处理空检查）
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayAnimation(UAnimMontage* Montage, FName SectionName = NAME_None, float PlayRate = 1.0f);

	// 多个section动画播放接口
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayAnimationWithSections(UAnimMontage* Montage, const TArray<FName>& SectionSequence, float PlayRate = 1.0f);

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

	// 当前Section
	UPROPERTY(BlueprintReadOnly, Category="Animation", meta=(AllowPrivateAccess="true"))
	int32 CurrentSectionIndex = -1;

	// 连招超时定时器
	FTimerHandle ComboTimerHandle;
	static constexpr float ComboTimeout = 1.0f;

	// 定时器回调
	void OnComboTimeout();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	FCharacterAnimation AnimationConfig;

	// UFUNCTION：绑定到动画实例的委托
	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
