// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GA_Dash.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/AlexAttributeSet.h"

UGA_Dash::UGA_Dash()
{
	// 默认Tag：这个技能属于"Ability.Dash"
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Dash")));

	// 默认阻挡Tag：有这些Tag时不能释放
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Climbing")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Attacking")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.WallRunning")));
}

bool UGA_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 先调用父类检查（包括Cost检查：DashCharges够不够）
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 2. 检查是否在空中（Falling）
	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        return false;
    }
	const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    bool bIsFalling = (MoveComp && MoveComp->MovementMode == MOVE_Falling);
    
    return bIsFalling;
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. 提交资源消耗（自动扣除DashCharges）
	// 如果Commit失败（比如Charges=0），会自动EndAbility    
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
        return;
    }

	// 获取Character（AvatarActor就是Alex）
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 添加"正在冲刺"Tag（阻止攀爬、攻击等）
	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));

	// 3. 执行位移
	FVector DashDir = Character->GetActorForwardVector();
	Character->GetCharacterMovement()->Velocity = DashDir * DashSpeed;

	// 4. 播放动画
	if (DashMontage)
	{
		Character->PlayAnimMontage(DashMontage);
	}

	// 5. 设置Timer，时间到自动结束
	GetWorld()->GetTimerManager().SetTimer(
		DashTimerHandle,
		this,
		&UGA_Dash::OnDashTimeExpired,
		DashDuration,
		false
	);
}

void UGA_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 清理Tag（必须在Super之前做，因为Super之后ActorInfo可能无效）
	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
	}

	// 获取Character并减速（你原来的逻辑）
    ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->GetCharacterMovement()->Velocity *= 0.2f;
    }

	// 清理Timer
	GetWorld()->GetTimerManager().ClearTimer(DashTimerHandle);

	// 调用父类（必须！否则内存泄漏）
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dash::OnDashTimeExpired()
{
	// Timer回调：主动结束Ability
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
