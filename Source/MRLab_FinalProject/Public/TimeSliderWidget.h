// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeSliderWidget.generated.h"

class USlider;
class UTextBlock;
class UButton;
class ASolarSystemManager;

/**
 * UMG widget — full-width time slider overlaid at the bottom of the screen.
 *
 * Slider range:  0.0 = −50 years,  0.5 = today (baseline),  1.0 = +50 years.
 * Step size: 0.01 = 1 year per notch.
 *
 * BindWidget names (must match exactly in WBP_TimeSlider):
 *   TimeSlider       — USlider
 *   SimulatedDateText — UTextBlock  (shows the currently-selected date)
 *   GoToNowButton    — UButton      (resets slider to center / today)
 */
UCLASS()
class MRLAB_FINALPROJECT_API UTimeSliderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	USlider* TimeSlider;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SimulatedDateText;

	UPROPERTY(meta = (BindWidget))
	UButton* GoToNowButton;

protected:
	virtual void NativeConstruct() override;

private:
	TWeakObjectPtr<ASolarSystemManager> Manager;

	// Baseline UTC date captured when the widget is constructed (matches manager's BaseAstroTime).
	FDateTime BaseDateTimeUtc;

	UFUNCTION()
	void OnSliderValueChanged(float Value);

	UFUNCTION()
	void OnGoToNowClicked();

	// Compute and set SimulatedDateText from an offset in years.
	void UpdateDateDisplay(float OffsetYears);
};
