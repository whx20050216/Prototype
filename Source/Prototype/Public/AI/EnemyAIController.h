// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class PROTOTYPE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
    virtual void BeginPlay() override;

	// ºÚ°å×é¼þ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	UBlackboardComponent* BlackboardComp;
};
