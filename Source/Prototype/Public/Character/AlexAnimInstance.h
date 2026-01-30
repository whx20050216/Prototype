// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AlexAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPE_API UAlexAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	class AAlexCharacter* AlexCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	class UCharacterMovementComponent* CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Glide")
	bool bIsGliding;

	UPROPERTY(BlueprintReadOnly, Category = "Glide")
	float GlideTilt;// «„–±÷µ£¨∑∂Œß -MaxGlideTiltAngle µΩ +MaxGlideTiltAngle

	UPROPERTY(BlueprintReadOnly, Category = "Climb")
	bool bIsClimbing;

	UPROPERTY(BlueprintReadOnly, Category = "Climb")
	float ClimbHorizontal;

	UPROPERTY(BlueprintReadOnly, Category = "Climb")
	float ClimbVertical;

	UPROPERTY(BlueprintReadOnly, Category = "WallRun")
	bool bIsWallRunning;

	UPROPERTY(BlueprintReadOnly, Category = "WallRun")
	FVector WallRunDirection;
};
