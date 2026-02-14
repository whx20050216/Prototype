// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/AnimNotify_MeleeDamageEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UAnimNotify_MeleeDamageEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	if (AActor* Owner = MeshComp->GetOwner())
    {
        if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
        {
            FGameplayEventData EventData;
            EventData.Instigator = Owner;
            EventData.Target = Owner;
            
            ASC->HandleGameplayEvent(
                FGameplayTag::RequestGameplayTag(FName("Event.MeleeDamage")),
                &EventData
            );
        }
    }
}
