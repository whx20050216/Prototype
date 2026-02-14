// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/AnimNotify_OpenComboWindow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UAnimNotify_OpenComboWindow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	UE_LOG(LogTemp, Warning, TEXT(">>> OpenComboWindow Notify CALLED"));

    if (!MeshComp)
    {
		UE_LOG(LogTemp, Error, TEXT("MeshComp is NULL!"));
        return;
    }

	AActor* Owner = MeshComp->GetOwner();
    UE_LOG(LogTemp, Warning, TEXT("Owner: %s"), Owner ? *Owner->GetName() : TEXT("NULL"));
    
    if (!Owner) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    UE_LOG(LogTemp, Warning, TEXT("ASC: %s"), ASC ? TEXT("Valid") : TEXT("NULL"));
    
    if (!ASC) return;
    
    FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.OpenComboWindow"));
    UE_LOG(LogTemp, Warning, TEXT("Sending Tag: %s"), *EventTag.ToString());
    
    FGameplayEventData EventData;
    EventData.Instigator = Owner;
    EventData.Target = Owner;
    
    ASC->HandleGameplayEvent(EventTag, &EventData);
    UE_LOG(LogTemp, Warning, TEXT(">>> Event SENT!"));
}
