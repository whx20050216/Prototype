// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"
#include "ActorComponent/AttributeComponent.h"
#include "Components/WidgetComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	LockPriority = 10.f;
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

void ABaseCharacter::PlayMontageSection(UAnimMontage* Montage, const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
}