// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/CharacterTypes.h"
#include "WallDetectionComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPE_API UWallDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWallDetectionComponent();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trace")
    bool bCanWallRun;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trace")
    bool bCanVault;

	UFUNCTION(BlueprintCallable, Category="Trace")
	void UpdateDetection(); 

	UFUNCTION(BlueprintCallable, Category="Trace")
	void RightCapsuleTrace();

	UFUNCTION(BlueprintCallable, Category="Trace")
	void LeftCapsuleTrace();

	UFUNCTION(BlueprintCallable, Category="Trace")
	void UpperTrace();

	UFUNCTION(BlueprintCallable, Category="Trace Results")
    const FHitResult& GetRightCapsuleHit() const { return RightCapsuleHit; }

    UFUNCTION(BlueprintCallable, Category="Trace Results")
    const FHitResult& GetLeftCapsuleHit() const { return LeftCapsuleHit; }

    UFUNCTION(BlueprintCallable, Category="Trace Results")
	const FHitResult& GetUpToDownHit() const { return UpToDownHit; }

	UFUNCTION(BlueprintCallable, Category="Trace Results")
	const FHitResult& GetBottomHit() const { return BottomHit; }

	UFUNCTION(BlueprintCallable, Category="Trace Results")
	const FHitResult& GetMiddleHit() const { return MiddleHit; }

	UFUNCTION(BlueprintCallable, Category="Trace Results")
	const FHitResult& GetTopHit() const { return TopHit; }

	UFUNCTION(BlueprintCallable, Category="Trace Results")
	const FHitResult& GetUpperHit() const { return UpperHit; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace")
    float LineTraceLength = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace")
	float UpperTraceLength = 150.f;

	UPROPERTY(BlueprintReadOnly, Category="Trace")
	FVector WallNormal;

	UPROPERTY(BlueprintReadOnly, Category="Trace")
	float ActorToWallDistance;

	UPROPERTY(BlueprintReadOnly, Category="Trace")
	FVector MiddlePointLocation;

	UPROPERTY(EditAnywhere, Category="Trace")
	float CapsuleTraceLength = 100.f;

	UPROPERTY(BlueprintReadOnly, Category="Trace")
	float CapsuleTraceRadius = 30.f;

	UPROPERTY(BlueprintReadOnly, Category="Trace")
	float CapsuleHalfHeight = 90.f;
	
protected:
	UPROPERTY()
	FHitResult UpperHit;

	UPROPERTY()
	FHitResult MiddleHit;

	UPROPERTY()
	FHitResult TopHit;

	UPROPERTY()
	FHitResult BottomHit;

	UPROPERTY()
	FHitResult RightCapsuleHit;

	UPROPERTY()
	FHitResult LeftCapsuleHit;

	UPROPERTY()
	FHitResult UpToDownHit;
	
private:
	UPROPERTY()
	EDetectionResult DetectionResult = EDetectionResult::Clear;

	UPROPERTY()
	TObjectPtr<ACharacter> CharacterOwner;

	UPROPERTY()
	bool bIsTracing = false;

	void UpToDownTrace();

	void BottomTrace();

	void TopTrace();

	void MiddleTrace();

	void EvaluateResults();

	void CapsuleTrace(const FVector& Direction, FHitResult& OutHitResult, float CapsuleRadius, float CapsuleHalf, float TraceLength, FColor DebugColor);
};
