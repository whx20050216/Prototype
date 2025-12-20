// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/TargetableInterface.h"
#include "BaseCharacter.generated.h"

class UAttributeComponent;
class UWidgetComponent;

UCLASS()
class PROTOTYPE_API ABaseCharacter : public ACharacter, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	virtual FVector GetTargetLocation_Implementation() const override;
    virtual float GetLockPriority_Implementation() const override;
    virtual void OnLocked_Implementation(AController* Locker) override;
    virtual void OnUnlocked_Implementation(AController* Unlocker) override;

	// 子类可以Override的配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
    float LockPriority = 10.f;  // 默认10，Boss可以Override成100

protected:
	virtual void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName);

};
