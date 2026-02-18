// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "PrototypeSaveGame.generated.h"

UCLASS()
class PROTOTYPE_API UPrototypeSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	// 已解锁的形态
	UPROPERTY()
	TSet<FGameplayTag> SavedUnlockedForms;

	// 当前血量
	UPROPERTY()
	float SavedHealth = 100.f;
};
