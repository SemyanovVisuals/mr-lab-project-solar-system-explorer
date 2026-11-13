// Fill out your copyright notice in the Description page of Project Settings.

#include "TimeSliderWidget.h"
#include "SolarSystemManager.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

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

    // Initialise slider as a read-only indicator.
    if (TimeSlider)
    {
        TimeSlider->SetMinValue(0.0f);
        TimeSlider->SetMaxValue(1.0f);
        TimeSlider->SetValue(0.5f);
        TimeSlider->SetLocked(true);
    }

    // Show today's date immediately.
    UpdateDateDisplay(0.0, true);
}

void UTimeSliderWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!Manager.IsValid()) return;

    const double OffsetYears = Manager->GetSimulatedOffsetYears();
    const double Speed = InDeltaTime > 0.0f ? FMath::Abs(OffsetYears - LastOffsetYears) / InDeltaTime : 0.0;
    LastOffsetYears = OffsetYears;

    // Map [−Max, +Max] years → [0, 1].
    if (TimeSlider)
    {
        TimeSlider->SetValue(0.5f + static_cast<float>(OffsetYears / (2.0 * ASolarSystemManager::MaxOffsetYears)));
    }

    // Minutes are only a blur when scrolling fast.
    UpdateDateDisplay(OffsetYears, Speed < TimeDisplaySpeedThreshold);
}

void UTimeSliderWidget::UpdateDateDisplay(double OffsetYears, bool bShowTime)
{
    if (!SimulatedDateText) return;

    const double OffsetDays = OffsetYears * 365.25;
    const FDateTime SimDate = BaseDateTimeUtc + FTimespan::FromDays(OffsetDays);

    const int32 Month = FMath::Clamp(SimDate.GetMonth(), 1, 12);
    FString Label = FString::Printf(
        TEXT("%s %d, %d"),
        MonthNames[Month - 1],
        SimDate.GetDay(),
        SimDate.GetYear()
    );

    if (bShowTime)
    {
        Label += FString::Printf(TEXT("  %02d:%02d UTC"), SimDate.GetHour(), SimDate.GetMinute());
    }

    SimulatedDateText->SetText(FText::FromString(Label));
}
