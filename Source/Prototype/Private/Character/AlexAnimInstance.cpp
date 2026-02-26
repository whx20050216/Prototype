// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AlexAnimInstance.h"
#include "Character/AlexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ActorComponent/WallDetectionComponent.h"

void UAlexAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	AlexCharacter = Cast<AAlexCharacter>(TryGetPawnOwner());
	if (AlexCharacter)
	{
		CharacterMovement = AlexCharacter->GetCharacterMovement();
	}
}

void UAlexAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!AlexCharacter) return;

	if (CharacterMovement)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(CharacterMovement->Velocity);
		IsFalling = CharacterMovement->IsFalling();
	}

	AimPitch = AlexCharacter->GetAimPitch();
	AimYaw = AlexCharacter->GetAimYaw();

	bIsWallRunning = AlexCharacter->GetbIsWallRunning();
}
