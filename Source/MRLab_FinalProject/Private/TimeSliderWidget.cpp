// Fill out your copyright notice in the Description page of Project Settings.

#include "TimeSliderWidget.h"
#include "SolarSystemManager.h"
#include "Components/Slider.h"
#include "Kismet/GameplayStatics.h"

void UTimeSliderWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Center the slider at startup (value 0.5 = today).
	if (TimeSlider)
	{
		TimeSlider->SetValue(0.5f);
		TimeSlider->OnValueChanged.AddDynamic(this, &UTimeSliderWidget::OnSliderValueChanged);
	}

	// Find the solar system manager in the current level.
	AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), ASolarSystemManager::StaticClass());
	Manager = Cast<ASolarSystemManager>(Found);

	if (!Manager.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("TimeSliderWidget: ASolarSystemManager not found in level."));
	}
}

void UTimeSliderWidget::OnSliderValueChanged(float Value)
{
	if (!Manager.IsValid()) return;

	// Map [0, 1] → [-100, +100] years.
	const float OffsetYears = (Value - 0.5f) * 200.0f;
	Manager->SetSimulatedOffsetYears(OffsetYears);
}
