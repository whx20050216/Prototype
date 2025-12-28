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

void ABaseCharacter::PlayAnimation(UAnimMontage* Montage, FName SectionName, float PlayRate)
{
	PlayMontageSection(Montage, SectionName, PlayRate);
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
