// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"
#include "ActorComponent/AttributeComponent.h"
#include "Components/WidgetComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	LockPriority = 10.f;
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 绑定动画结束委托到动画实例
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &ABaseCharacter::HandleMontageEnded);
	}
}

void ABaseCharacter::OnComboTimeout()
{
	CurrentSectionIndex = -1;
	GetWorldTimerManager().ClearTimer(ComboTimerHandle);
}

void ABaseCharacter::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	OnMontageEnded.Broadcast(Montage, bInterrupted);
}

FVector ABaseCharacter::GetTargetLocation_Implementation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 150.f);
}

float ABaseCharacter::GetLockPriority_Implementation() const
{
	return LockPriority;
}

void ABaseCharacter::OnLocked_Implementation(AController* Locker)
{
}

void ABaseCharacter::OnUnlocked_Implementation(AController* Unlocker)
{
}

void ABaseCharacter::AttackEnd_Implementation()
{
	// 只有Sequential模式才处理
    if (AnimationConfig.PlayMode != ESectionPlayMode::Sequential)
    {
        return;
    }

	// 获取下一个索引
    int32 NextIndex = GetNextSectionIndex();

	if (NextIndex == -1)
	{
		GetWorldTimerManager().ClearTimer(ComboTimerHandle);
		CurrentSectionIndex = -1;
	}
	else
	{
		CurrentSectionIndex = NextIndex;
		FName NextSection = AnimationConfig.SectionSequence[CurrentSectionIndex];

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_JumpToSection(NextSection, AnimationConfig.Montage);
		}
		//重置定时器
		GetWorldTimerManager().ClearTimer(ComboTimerHandle);
        GetWorldTimerManager().SetTimer(ComboTimerHandle, this, &ABaseCharacter::OnComboTimeout, ComboTimeout, false);
	}
}

void ABaseCharacter::PlayMontageSection(UAnimMontage* Montage, const FName& SectionName, float PlayRate)
{
	if (!Montage) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	AnimInstance->Montage_Play(Montage, PlayRate);

	if (SectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
}

void ABaseCharacter::PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionSequence, float PlayRate)
{
	if (!Montage || SectionSequence.Num() == 0) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	int32 RandomIndex = FMath::RandRange(0, SectionSequence.Num() - 1);
	FName SectionName = SectionSequence[RandomIndex];

	PlayMontageSection(Montage, SectionName, PlayRate);
}

void ABaseCharacter::PlaySequentialMontageSections(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || AnimationConfig.SectionSequence.Num() == 0) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

	CurrentSectionIndex = 0;
    FName FirstSection = AnimationConfig.SectionSequence[CurrentSectionIndex];
	PlayMontageSection(Montage, FirstSection, PlayRate);

	GetWorldTimerManager().SetTimer(ComboTimerHandle, this, &ABaseCharacter::OnComboTimeout, ComboTimeout, false);
}

int32 ABaseCharacter::GetNextSectionIndex() const
{
	if (CurrentSectionIndex < 0 || CurrentSectionIndex >= AnimationConfig.SectionSequence.Num() - 1)
	{
		return -1;
	}
	return CurrentSectionIndex + 1;
}

void ABaseCharacter::PlayAnimation(UAnimMontage* Montage, FName SectionName, float PlayRate)
{
	PlayMontageSection(Montage, SectionName, PlayRate);
}

void ABaseCharacter::PlayAnimationWithSections(UAnimMontage* Montage, const TArray<FName>& SectionSequence, float PlayRate)
{
	if (!Montage) return;

	if (AnimationConfig.PlayMode == ESectionPlayMode::Random)
	{
		PlayRandomMontageSection(Montage, SectionSequence, PlayRate);
	}
	else if (AnimationConfig.PlayMode == ESectionPlayMode::Sequential)
	{
		PlaySequentialMontageSections(Montage, PlayRate);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid PlayMode"));
	}
}

bool ABaseCharacter::IsPlayingMontage(UAnimMontage* Montage, FName SectionName) const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance || !Montage) return false;

	if (AnimInstance->Montage_IsActive(Montage))
	{
		if (SectionName != NAME_None)
		{
			return AnimInstance->Montage_GetCurrentSection() == SectionName;
		}
		return true;
	}
	return false;
}

void ABaseCharacter::StopAllMontages(float BlendOutTime)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(BlendOutTime);
	}
}
