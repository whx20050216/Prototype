// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"



UCLASS()
class PROTOTYPE_API APrototypePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

	//Ëø¶¨¹ÜÀíÆ÷

};
