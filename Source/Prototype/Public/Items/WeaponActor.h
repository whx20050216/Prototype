// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponActor.generated.h"

UCLASS()
class PROTOTYPE_API AWeaponActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponActor();
	
	UPROPERTY(VisibleAnywhere, Category="Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};
