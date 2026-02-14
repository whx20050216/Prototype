// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GA_AlexAttack.h"
#include "Character/AlexCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_AlexAttack::UGA_AlexAttack()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Attacking")));

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Stunned")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dashing")));
}

bool UGA_AlexAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UE_LOG(LogTemp, Error, TEXT(">>> CanActivateAbility Trigger!"));
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;

	return true;
}

void UGA_AlexAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Error, TEXT(">>> ActivateAbility Trigger!"));
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AlexChar = Cast<AAlexCharacter>(ActorInfo->AvatarActor);
	if (!AlexChar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimInstance* Instance = AlexChar->GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = AlexChar->GetCurrentMorphConfig().ComboMontage;

	if (Instance && Montage && Instance->Montage_IsPlaying(Montage))
	{
		const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
		int32 CurrentComboStep = AlexChar->GetAttackComboStep();
		int32 NextStep = CurrentComboStep + 1;
		if (Config.LightComboSections.IsValidIndex(NextStep))
		{
			// 有下一段，尝试推进连击
            TryAdvanceCombo();
		}
	}
	else
	{
		// 没播放 → 新攻击，重置 Step 为 0
        AlexChar->ResetAttackCombo();
        PlayCurrentComboSection();
	}
}

void UGA_AlexAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Error, TEXT(">>> EndAbility Trigger!"));
	// 1. 防递归（清理 Task 时可能触发回调导致重入）
	if (bIsEndingAbility) return;
	bIsEndingAbility = true;

	CloseComboWindow();

	if (MontageTask)
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnBlendOut.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
        MontageTask->ExternalCancel();
        MontageTask = nullptr;
    }
	if (DamageEventTask)
	{
		DamageEventTask->EventReceived.RemoveAll(this);
		DamageEventTask->ExternalCancel();
		DamageEventTask = nullptr;
	}
	if (ComboWindowEventTask)
    {
        ComboWindowEventTask->EventReceived.RemoveAll(this);
        ComboWindowEventTask->ExternalCancel();
        ComboWindowEventTask = nullptr;
    }
	if (AlexChar && !bIsAdvancingCombo)
    {
        AlexChar->ResetAttackCombo();
    }
	if (InputBufferTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(InputBufferTimer);
    }
    bComboWindowOpen = false;
    bIsAdvancingCombo = false;
    bInputBuffered = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bIsEndingAbility = false;
}

void UGA_AlexAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 如果已经在窗口期内，不需要处理（WaitInputPress Task 会接管）
	if (bComboWindowOpen) return;

	// 如果 Ability 没激活，不处理
	if (!IsActive()) return;

	// 预输入缓冲：玩家按得稍微早一点，帮他存住
	bInputBuffered = true;

	// 0.15 秒后过期（如果这段时间内没开窗口，就丢弃这次输入）
	GetWorld()->GetTimerManager().SetTimer(
		InputBufferTimer,
		[this]() { bInputBuffered = false; },
		0.15f,
		false
	);
}

void UGA_AlexAttack::PlayCurrentComboSection()
{
	if (!AlexChar)return;

	// 1. 清理旧 Task
	if (MontageTask)
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnBlendOut.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
        MontageTask->ExternalCancel();
        MontageTask = nullptr;
    }
    if (DamageEventTask)
    {
        DamageEventTask->EventReceived.RemoveAll(this);
        DamageEventTask->ExternalCancel();
        DamageEventTask = nullptr;
    }
    if (ComboWindowEventTask)
    {
        ComboWindowEventTask->EventReceived.RemoveAll(this);
        ComboWindowEventTask->ExternalCancel();
        ComboWindowEventTask = nullptr;
    }

	// 2. 获取当前形态配置
	const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
	if (!Config.ComboMontage || Config.LightComboSections.Num() == 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	int32 CurrentStep = AlexChar->GetAttackComboStep();
	if (!Config.LightComboSections.IsValidIndex(CurrentStep))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 3. 获取当前段名（如 "Jab1", "Jab2"）
	FName SectionName = Config.LightComboSections[CurrentStep];

	// 4. 创建 MontageTask 播放动画
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Config.ComboMontage,
		1.f,
		SectionName,
		false
	);

	// 5. 绑定蒙太奇生命周期回调
	MontageTask->OnCompleted.AddDynamic(this, &UGA_AlexAttack::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_AlexAttack::OnMontageBlendOut);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_AlexAttack::OnMontageInterrupted); // 打断按 BlendOut 处理
    MontageTask->OnCancelled.AddDynamic(this, &UGA_AlexAttack::OnMontageInterrupted);   // 取消按 BlendOut 处理
    MontageTask->ReadyForActivation();

	// 6. 创建 DamageEventTask 监听伤害事件（AnimNotify 发送 Event.MeleeDamage）
	DamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		FGameplayTag::RequestGameplayTag(FName("Event.MeleeDamage"))
	);
	DamageEventTask->EventReceived.AddDynamic(this, &UGA_AlexAttack::OnDamageEventReceived);
	DamageEventTask->ReadyForActivation();

	// 7. 创建 ComboWindowEventTask 监听连击窗口开启（AnimNotify 发送 Event.OpenComboWindow）
	ComboWindowEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		FGameplayTag::RequestGameplayTag(FName("Event.OpenComboWindow"))
	);
	ComboWindowEventTask->EventReceived.AddDynamic(this, &UGA_AlexAttack::OnComboWindowEventReceived);
	ComboWindowEventTask->ReadyForActivation();
}

void UGA_AlexAttack::OpenComboWindow()
{
	// 1. 保险：如果 Ability 已经结束了（比如被击飞打断），不开启窗口
	if (!IsActive()) return;

	// 2. 标记窗口开启（给 InputPressed 虚函数检查用）
	bComboWindowOpen = true;

	// 如果有预输入，立即触发连击，不创建 Task 监听
	if (bInputBuffered)
	{
		bInputBuffered = false;
		if (InputBufferTimer.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(InputBufferTimer);
		}
		// 先检查是否真的能连击（避免 TryAdvanceCombo 直接 return 导致状态错误）
		const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
		int32 NextStep = AlexChar->GetAttackComboStep() + 1;
		
		if (Config.LightComboSections.IsValidIndex(NextStep))
		{
			// 能连击，TryAdvanceCombo 会处理 CloseComboWindow
			TryAdvanceCombo();
		}
		else
		{
			// 不能连击（最后一段），重置窗口状态，让动画自然结束
			bComboWindowOpen = false;
		}
		return;
	}

	// 3. 创建输入监听 Task：监听玩家按下攻击键
	// 参数 false 表示"不消耗输入事件"（允许其他系统也收到）
	InputPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	InputPressTask->OnPress.AddDynamic(this, &UGA_AlexAttack::OnInputPressed);
	InputPressTask->ReadyForActivation();

	// 4. 启动 0.5 秒定时器：超时自动关闭窗口（玩家没按键）
	GetWorld()->GetTimerManager().SetTimer(
        ComboWindowTimer,           // 成员变量 FTimerHandle
        this,                       // 回调对象
        &UGA_AlexAttack::CloseComboWindow,  // 超时回调
        0.5f,                       // 窗口期持续时间（秒）
        false                       // 不循环，只执行一次
    );
}

void UGA_AlexAttack::CloseComboWindow()
{
	bComboWindowOpen = false;

	if (InputPressTask)
	{
		InputPressTask->ExternalCancel();
		InputPressTask = nullptr;
	}

	if (ComboWindowTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimer);
	}
}

void UGA_AlexAttack::TryAdvanceCombo()
{
	if (!AlexChar || !IsActive()) return;

	const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
	int32 NextStep = AlexChar->GetAttackComboStep() + 1;

	if (!Config.LightComboSections.IsValidIndex(NextStep)) return;

	bIsAdvancingCombo = true;		// 标记"正在衔接中"

	CloseComboWindow();

	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnBlendOut.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
        MontageTask->ExternalCancel();
        MontageTask = nullptr;
	}

	AlexChar->AdvanceAttackCombo();

	PlayCurrentComboSection();

	bIsAdvancingCombo = false;
}

void UGA_AlexAttack::PerformDamageCheck()
{
	if (!AlexChar) return;

	// 1. 获取当前形态配置（伤害、范围、角度）
	const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();

	// 2. 设置检测参数（胸口高度，防止打地面）
	FVector Start = AlexChar->GetActorLocation() + FVector(0, 0, 50.f);
	float Radius = Config.MeleeRadius;
	float HalfAngle = Config.AttackAngle / 2.f; // 半角（如 90° 范围就是 ±45°）

	// 3. 球形检测（Sweep）
	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start, Start,					// 起点终点相同，检测当前位置
		FQuat::Identity,
		ECC_Pawn,						// 检测 Pawn 层（敌人、NPC）
		Sphere
	);

	// 4. 处理命中
	if (bHit)
	{
		for (auto& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == AlexChar) continue;

			// 5. 扇形角度检查（只打前方的敌人）
			FVector ToTarget = (HitActor->GetActorLocation() - AlexChar->GetActorLocation()).GetSafeNormal();
			float Dot = FVector::DotProduct(AlexChar->GetActorForwardVector(), ToTarget);
			float AngleDeg = FMath::Acos(Dot) * (180.f / PI); // 转成角度

			// 在攻击角度范围内才造成伤害（如 90° 范围就是 ±45°）
			if (AngleDeg <= HalfAngle)
			{
				// 6. 应用伤害
				UGameplayStatics::ApplyDamage(
					HitActor,
					Config.Damage,
					AlexChar->GetController(),
					AlexChar,
					UDamageType::StaticClass()
				);

				// 7. 如果不是多段攻击武器（如锤子），命中一个就停
				if (!Config.bIsMultiHit)
				{
					break;
				}
			}
		}
	}
	#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(GetWorld(), Start, Radius, 12, FColor::Red, false, 0.5f);
	#endif
}

void UGA_AlexAttack::OnMontageCompleted()
{
	UE_LOG(LogTemp, Error, TEXT(">>> OnMontageCompleted Trigger!"));
	// 如果窗口还开着，先关了
    if (bComboWindowOpen)
    {
        CloseComboWindow();
    }
	// 动画完整播完了（最后一段攻击结束）
    // 参数：true = 复制到服务器，false = 不是被取消（是正常完成）
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AlexAttack::OnMontageBlendOut()
{
	UE_LOG(LogTemp, Error, TEXT(">>> OnMontageBlendOut Trigger!"));
	if (bIsAdvancingCombo) return;

	if (AlexChar)
	{
		const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
        int32 NextStep = AlexChar->GetAttackComboStep() + 1;
		bool bHasNext = Config.LightComboSections.IsValidIndex(NextStep);
		// 有下一段且窗口开着 → 继续等输入
        if (bHasNext && bComboWindowOpen) return;
		// 否则（无下一段 或 窗口已关）→ 结束
	}

    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_AlexAttack::OnMontageInterrupted()
{
    // 被打断/取消时立即结束，不管窗口状态
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_AlexAttack::OnDamageEventReceived(FGameplayEventData EventData)
{
	PerformDamageCheck();
}

void UGA_AlexAttack::OnComboWindowEventReceived(FGameplayEventData EventData)
{
	if (IsActive()) OpenComboWindow();
}

void UGA_AlexAttack::OnInputPressed(float TimeWaited)
{
	if (bComboWindowOpen) TryAdvanceCombo();
}
