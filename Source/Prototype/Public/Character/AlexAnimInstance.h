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
	float GlideTilt;// 倾斜值，范围 -MaxGlideTiltAngle 到 +MaxGlideTiltAngle

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

	UPROPERTY(BlueprintReadOnly)
	bool bIsHoldingGun = false;  // 是否持枪

	UPROPERTY(BlueprintReadOnly)
	float AimPitch = 0.f;  // 视角俯仰角度

	UPROPERTY(BlueprintReadOnly)
	float AimYaw = 0.f;  // 视角偏航角度

	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouching;  // 是否蹲下
};