// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_AlexAttack.generated.h"

class AAlexCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class PROTOTYPE_API UGA_AlexAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_AlexAttack();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;


protected:
	// 状态标记（GA 实例内有效）
	bool bComboWindowOpen = false;        // 是否在输入窗口期
	bool bIsAdvancingCombo = false;       // 是否正在衔接下一段（防 BlendOut 误结束）

	UPROPERTY()
	bool bIsEndingAbility = false;        // 是否正在结束 Ability（防止重复调用）

	UPROPERTY()
	bool bInputBuffered = false;

	UPROPERTY()
	FTimerHandle ComboWindowTimer;

	UPROPERTY()
	FTimerHandle InputBufferTimer;

	// 缓存
	TObjectPtr<AAlexCharacter> AlexChar;

	// AbilityTasks（必须持有才能清理）
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
	TObjectPtr<UAbilityTask_WaitGameplayEvent> DamageEventTask;
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboWindowEventTask;
	TObjectPtr<UAbilityTask_WaitInputPress> InputPressTask;

	// 函数
	void PlayCurrentComboSection();       // 播放当前段动画
	void OpenComboWindow();               // 开启输入监听
	void CloseComboWindow();              // 关闭输入监听
	void TryAdvanceCombo();               // 尝试进入下一段
	void PerformDamageCheck();            // 球形检测伤害
	
	// 回调（UFUNCTION）
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnDamageEventReceived(FGameplayEventData EventData);

	UFUNCTION()
	void OnComboWindowEventReceived(FGameplayEventData EventData);

	UFUNCTION()
	void OnInputPressed(float TimeWaited);
};
