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
}

void UHealthWidget::OnHealthChanged(float CurrentHealth, float MaxHealth, float Delta)
{
	UpdateHealthDisplay(CurrentHealth, MaxHealth);

}

void UHealthWidget::UpdateHealthDisplay(float CurrentHealth, float MaxHealth)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}
	if (HealthText)
    {
        HealthText->SetText(FText::Format(
            INVTEXT("{0} / {1}"),
            FMath::CeilToInt(CurrentHealth),
            FMath::CeilToInt(MaxHealth)
        ));
    }
}

void UHealthWidget::NativeDestruct()
{
	if (BoundAttributeComp)
    {
        BoundAttributeComp->OnHealthChanged.RemoveDynamic(this, &UHealthWidget::OnHealthChanged);
        BoundAttributeComp = nullptr;
    }
    Super::NativeDestruct();
}
