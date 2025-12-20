// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/PrototypePlayerController.h"
#include "PlayerController/LockOnManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void APrototypePlayerController::BeginPlay()
{
	Super::BeginPlay();

	LockOnManager = NewObject<ULockOnManager>(this, TEXT("LockOnManager"));

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }
}

void APrototypePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &APrototypePlayerController::Input_ToggleLockOn);
	}
}

void APrototypePlayerController::Input_ToggleLockOn()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("LockOn"));
	if (LockOnManager)
	{
		LockOnManager->ToggleLockOn();
	}
}
