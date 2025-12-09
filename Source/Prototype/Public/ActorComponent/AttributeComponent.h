// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPE_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttributeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //血量变化委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth, float, Delta);

    //死亡委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

	UPROPERTY(BlueprintAssignable, Category="Attributes")
    FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Attributes")
    FOnDeath OnDeath;

	// 受击/治疗接口
    UFUNCTION(BlueprintCallable, Category="Attributes")
    void ApplyHealthChange(float Delta);

	// 获取当前血量（供UI读取）
    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Attributes")
    float GetMaxHealth() const { return MaxHealth; }

    
protected:
	virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
    float Health = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
    float MaxHealth = 100.f;

};
