// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/HealthWidget.h"
#include "ActorComponent/AttributeComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHealthWidget::BindToAttributeComponent(UAttributeComponent* AttributeComp)
{
	if (!AttributeComp) return;

	BoundAttributeComp = AttributeComp;

    AttributeComp->OnHealthChanged.AddDynamic(this, &UHealthWidget::OnHealthChanged);
    UpdateHealthDisplay(AttributeComp->GetHealth(), AttributeComp->GetMaxHealth());

    AttributeComp->OnManaChanged.AddDynamic(this, &UHealthWidget::OnManaChanged);
    UpdateManaDisplay(AttributeComp->GetMana(), AttributeComp->GetMaxMana());
}

void UHealthWidget::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta)
{
	UpdateHealthDisplay(CurrentHealth, MaxHealth);

}

void UHealthWidget::OnManaChanged(float CurrentMana, float MaxMana, float Delta)
{
	UpdateManaDisplay(CurrentMana, MaxMana);
}

void UHealthWidget::UpdateHealthDisplay(float CurrentHealth, float MaxHealth)
{
	if (Bar_Health)
	{
		Bar_Health->SetPercent(CurrentHealth / MaxHealth);
	}
	if (Text_Health)
    {
            Text_Health->SetText(FText::Format(
            INVTEXT("{0} / {1}"),
            FMath::CeilToInt(CurrentHealth),
            FMath::CeilToInt(MaxHealth)
        ));
    }
}

void UHealthWidget::UpdateManaDisplay(float CurrentMana, float MaxMana)
{
	if (Bar_Mana)
	{
		Bar_Mana->SetPercent(CurrentMana / MaxMana);
	}
	if (Text_Mana)
    {
            Text_Mana->SetText(FText::Format(
            INVTEXT("{0} / {1}"),
            FMath::CeilToInt(CurrentMana),
            FMath::CeilToInt(MaxMana)
        ));
    }
}

void UHealthWidget::NativeDestruct()
{
	if (BoundAttributeComp)
    {
        BoundAttributeComp->OnHealthChanged.RemoveDynamic(this, &UHealthWidget::OnHealthChanged);
        BoundAttributeComp->OnManaChanged.RemoveDynamic(this, &UHealthWidget::OnManaChanged);
        BoundAttributeComp = nullptr;
    }
    Super::NativeDestruct();
}
