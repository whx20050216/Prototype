// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponActor.h"

AWeaponActor::AWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	// Ä¬ÈÏ¹Ø±ÕÅö×²£¨´¿ÊÓ¾õ£©
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}