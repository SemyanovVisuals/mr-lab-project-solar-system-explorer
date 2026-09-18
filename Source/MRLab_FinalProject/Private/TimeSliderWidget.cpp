// Fill out your copyright notice in the Description page of Project Settings.

#include "TimeSliderWidget.h"
#include "SolarSystemManager.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

// Total year span of the slider (centre ± HalfRange).
static constexpr float HalfRange = 50.0f;

// Short month name lookup.
static const TCHAR* MonthNames[] = {
    TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"),
    TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"),
    TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec")
};

void UTimeSliderWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Capture the current UTC date — used to compute the display label.
    BaseDateTimeUtc = FDateTime::UtcNow();

    // Find the solar system manager.
    AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), ASolarSystemManager::StaticClass());
    Manager = Cast<ASolarSystemManager>(Found);
    if (!Manager.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("TimeSliderWidget: ASolarSystemManager not found in level."));
    }

    // Initialise slider.
    if (TimeSlider)
    {
        TimeSlider->SetMinValue(0.0f);
        TimeSlider->SetMaxValue(1.0f);
        // One notch per year: 100-year total span → step = 0.01.
        TimeSlider->SetStepSize(0.01f);
        TimeSlider->SetValue(0.5f);
        TimeSlider->OnValueChanged.AddDynamic(this, &UTimeSliderWidget::OnSliderValueChanged);
    }

    // Initialise Go to Now button.
    if (GoToNowButton)
    {
        GoToNowButton->OnClicked.AddDynamic(this, &UTimeSliderWidget::OnGoToNowClicked);
    }

    // Show today's date immediately.
    UpdateDateDisplay(0.0f);
}

void UTimeSliderWidget::OnSliderValueChanged(float Value)
{
    if (!Manager.IsValid()) return;

    // Map [0, 1] → [−HalfRange, +HalfRange] years.
    const float OffsetYears = (Value - 0.5f) * (HalfRange * 2.0f);
    Manager->SetSimulatedOffsetYears(OffsetYears);
    UpdateDateDisplay(OffsetYears);
}

void UTimeSliderWidget::OnGoToNowClicked()
{
    // SetValue fires OnValueChanged, which updates planets + label.
    if (TimeSlider)
    {
        TimeSlider->SetValue(0.5f);
    }
    // If slider was already at 0.5 the delegate may not fire — force it.
    if (Manager.IsValid())
    {
        Manager->SetSimulatedOffsetYears(0.0f);
    }
    UpdateDateDisplay(0.0f);
}

void UTimeSliderWidget::UpdateDateDisplay(float OffsetYears)
{
    if (!SimulatedDateText) return;

    const double OffsetDays = static_cast<double>(OffsetYears) * 365.25;
    const FDateTime SimDate = BaseDateTimeUtc + FTimespan::FromDays(OffsetDays);

    const int32 Month = FMath::Clamp(SimDate.GetMonth(), 1, 12);
    const FString Label = FString::Printf(
        TEXT("%s %d, %d"),
        MonthNames[Month - 1],
        SimDate.GetDay(),
        SimDate.GetYear()
    );

    SimulatedDateText->SetText(FText::FromString(Label));
}
