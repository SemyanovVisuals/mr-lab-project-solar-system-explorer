// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeSliderWidget.generated.h"

class USlider;
class ASolarSystemManager;

/**
 * UMG widget that renders a full-width horizontal time slider at the bottom of the screen.
 * Moving the slider calls ASolarSystemManager::SetSimulatedOffsetYears() to reposition planets.
 *
 * Slider value 0.0 = -100 years, 0.5 = today (baseline), 1.0 = +100 years.
 *
 * In the WBP Blueprint, add a USlider widget and name it exactly "TimeSlider".
 */
UCLASS()
class MRLAB_FINALPROJECT_API UTimeSliderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Bound to the Slider widget inside WBP_TimeSlider. Name must match exactly.
	UPROPERTY(meta = (BindWidget))
	USlider* TimeSlider;

protected:
	virtual void NativeConstruct() override;

private:
	TWeakObjectPtr<ASolarSystemManager> Manager;

	UFUNCTION()
	void OnSliderValueChanged(float Value);
};
