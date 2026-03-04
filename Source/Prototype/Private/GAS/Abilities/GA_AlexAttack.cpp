// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GA_AlexAttack.h"
#include "Character/AlexCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/WeaponActor.h"

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

	if (AlexChar)
	{
		AlexChar->SetMovementLock(true);
		AlexChar->GetCharacterMovement()->StopMovementImmediately();
	}

	if (AlexChar->IsComboWindowOpen())
	{
		const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
		int32 CurrentComboStep = AlexChar->GetAttackComboStep();
		int32 NextStep = CurrentComboStep + 1;
		if (Config.LightComboSections.IsValidIndex(NextStep))
		{
			// 有下一段，尝试推进连击
			AlexChar->SetComboWindowOpen(false);
            TryAdvanceCombo();
			return;
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}
	else
	{
		// 不在窗口期 → 新攻击序列，重置连击
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
	if (CloseWindowEventTask)
    {
        CloseWindowEventTask->EventReceived.RemoveAll(this);
        CloseWindowEventTask->ExternalCancel();
        CloseWindowEventTask = nullptr;
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

	if (AlexChar)
	{
		AlexChar->SetMovementLock(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bIsEndingAbility = false;
}

void UGA_AlexAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 如果已经在窗口期内，直接触发连击！
	if (bComboWindowOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> InputPressed during window, advancing combo"));
        TryAdvanceCombo();
		return;
	}

	// 如果 Ability 没激活，不处理
	if (!IsActive()) return;

	// 预输入缓冲：玩家按得稍微早一点，帮他存住
	bInputBuffered = true;

	// 0.3 秒后过期（如果这段时间内没开窗口，就丢弃这次输入）
	GetWorld()->GetTimerManager().SetTimer(
		InputBufferTimer,
		[this]() { bInputBuffered = false; },
		0.3f,
		false
	);
}

void UGA_AlexAttack::PlayCurrentComboSection()
{
	if (!AlexChar)return;

	UE_LOG(LogTemp, Error, TEXT(">>> PlayCombo | Montage: %s | Sections: %d | CurrentStep: %d"),
        *GetNameSafe(AlexChar->GetCurrentMorphConfig().ComboMontage),
        AlexChar->GetCurrentMorphConfig().LightComboSections.Num(),
        AlexChar->GetAttackComboStep());

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
	if (CloseWindowEventTask)
	{
		CloseWindowEventTask->EventReceived.RemoveAll(this);
		CloseWindowEventTask->ExternalCancel();
		CloseWindowEventTask = nullptr;
	}

	// 2. 获取当前形态配置
	const FMorphConfig& Config = AlexChar->GetCurrentMorphConfig();
	if (!Config.ComboMontage || Config.LightComboSections.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT(">>> PlayCombo FAILED: ComboMontage is NULL!"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	int32 CurrentStep = AlexChar->GetAttackComboStep();
	if (!Config.LightComboSections.IsValidIndex(CurrentStep))
	{
		UE_LOG(LogTemp, Error, TEXT(">>> PlayCombo FAILED: Invalid Step %d, ArrayNum: %d"), 
            CurrentStep, Config.LightComboSections.Num());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 3. 获取当前段名（如 "Jab1", "Jab2"）
	FName SectionName = Config.LightComboSections[CurrentStep];
	UE_LOG(LogTemp, Error, TEXT(">>> PlayCombo | Step: %d | SectionName: %s"), 
        CurrentStep, *SectionName.ToString());
	if (!Config.ComboMontage->IsValidSectionName(SectionName))
    {
        UE_LOG(LogTemp, Error, TEXT(">>> PlayCombo FAILED: Section '%s' NOT FOUND in Montage '%s'!"), 
            *SectionName.ToString(), *Config.ComboMontage->GetName());
        // 列出所有可用 Section 供你核对
        for (int32 i = 0; i < Config.ComboMontage->CompositeSections.Num(); i++)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> Available Section[%d]: %s"), 
                i, *Config.ComboMontage->CompositeSections[i].SectionName.ToString());
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

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

	// 8. 创建 CloseWindowEventTask 监听窗口关闭
    CloseWindowEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        FGameplayTag::RequestGameplayTag(FName("Event.CloseComboWindow"))
    );
    CloseWindowEventTask->EventReceived.AddDynamic(this, &UGA_AlexAttack::OnCloseComboWindowEventReceived);
    CloseWindowEventTask->ReadyForActivation();
}

void UGA_AlexAttack::OpenComboWindow()
{
	// 1. 保险：如果 Ability 已经结束了（比如被击飞打断），不开启窗口
	if (!IsActive()) return;

	// 2. 标记窗口开启（给 InputPressed 虚函数检查用）
	if (AlexChar) AlexChar->SetComboWindowOpen(true);
	bComboWindowOpen = true;

	UE_LOG(LogTemp, Warning, TEXT(">>> OpenComboWindow | bInputBuffered: %s"), bInputBuffered ? TEXT("TRUE") : TEXT("FALSE"));

	// 如果有预输入，立即触发连击，不创建 Task 监听
	if (bInputBuffered)
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> Processing buffered input"));

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
			UE_LOG(LogTemp, Warning, TEXT(">>> Buffered input processed, TryAdvanceCombo called"));
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT(">>> No next step, continuing to create InputPressTask"));
	}
	UE_LOG(LogTemp, Warning, TEXT(">>> Window open, waiting for InputPressed"));
}

void UGA_AlexAttack::CloseComboWindow()
{
	if (AlexChar) AlexChar->SetComboWindowOpen(false);
	bComboWindowOpen = false;

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
		int32 i = 0;
		for (auto& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == AlexChar) continue;
			if (HitActor->IsA(AWeaponActor::StaticClass())) continue;// 忽略武器

			// 5. 扇形角度检查（只打前方的敌人）
			FVector ToTarget = (HitActor->GetActorLocation() - AlexChar->GetActorLocation()).GetSafeNormal();
			float Dot = FVector::DotProduct(AlexChar->GetActorForwardVector(), ToTarget);
			float AngleDeg = FMath::Acos(Dot) * (180.f / PI); // 转成角度

			// 在攻击角度范围内才造成伤害（如 90° 范围就是 ±45°）
			if (AngleDeg <= HalfAngle)
			{
				// 6. 应用伤害
				float ActualDamage = UGameplayStatics::ApplyDamage(
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
        if (bHasNext) return;
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

void UGA_AlexAttack::OnCloseComboWindowEventReceived(FGameplayEventData EventData)
{
	if (bComboWindowOpen && !bIsAdvancingCombo)  // 如果没在衔接中，就关闭
    {
        CloseComboWindow();
    }
}

void UGA_AlexAttack::OnInputPressed(float TimeWaited)
{
	if (bComboWindowOpen) TryAdvanceCombo();
}
