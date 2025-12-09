// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UAttributeComponent::ApplyHealthChange(float Delta)
{
	const float OldHealth = Health;
	Health = FMath::Clamp(Health + Delta, 0.0f, MaxHealth);

	const float ActualDelta = Health - OldHealth;
	// 广播变化（只有变化时才广播，节省性能）
    if (!FMath::IsNearlyEqual(OldHealth, Health))
    {
        OnHealthChanged.Broadcast(Health, MaxHealth, ActualDelta);
    }

	if (Health <= 0.0f)
	{
		OnDeath.Broadcast();
	}
}

