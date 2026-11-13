// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeSliderWidget.generated.h"

class USlider;
class UTextBlock;
class ASolarSystemManager;

/**
 * UMG widget — full-width time indicator overlaid at the bottom of the screen.
 *
 * Time is scrolled with the IA_TimeForward / IA_TimeBackward input actions; this widget
 * only mirrors the manager's simulated offset (read-only slider + date label).
 * Pressing both buttons at once jumps back to today.
 * Slider range:  0.0 = −50 years,  0.5 = today (baseline),  1.0 = +50 years.
 *
 * BindWidget names (must match exactly in WBP_TimeSlider):
 *   TimeSlider       — USlider      (locked; shows the current offset)
 *   SimulatedDateText — UTextBlock  (shows the currently-simulated date) */
UCLASS()
class MRLAB_FINALPROJECT_API UTimeSliderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	USlider* TimeSlider;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SimulatedDateText;

	// Above this scroll speed (years per second) the label drops the hh:mm. Default: 1 week/s.
	UPROPERTY(EditAnywhere, Category = "Time")
	double TimeDisplaySpeedThreshold = 7.0 / 365.25;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<ASolarSystemManager> Manager;

	// Baseline UTC date captured when the widget is constructed (matches manager's BaseAstroTime).
	FDateTime BaseDateTimeUtc;

	// Offset seen on the previous tick — used to measure the scroll speed.
	double LastOffsetYears = 0.0;

	// Compute and set SimulatedDateText from an offset in years.
	void UpdateDateDisplay(double OffsetYears, bool bShowTime);
};
