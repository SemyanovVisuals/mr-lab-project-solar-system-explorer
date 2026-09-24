// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetBase.h"
extern "C"
{
#include "../ExternalScripts/astronomy.h"
}
#include "SolarSystemManager.generated.h"

class UTimeSliderWidget;

UCLASS()
class MRLAB_FINALPROJECT_API ASolarSystemManager : public AActor
{
	GENERATED_BODY()

public:
	ASolarSystemManager();

	// Called by the time slider widget when the user drags the slider.
	// OffsetYears: 0 = today, -100 = 100 years in the past, +100 = 100 years in the future.
	UFUNCTION(BlueprintCallable, Category = "Solar System")
	void SetSimulatedOffsetYears(float OffsetYears);

	// Widget Blueprint class to instantiate. Set this in BP_SolarSystemManager class defaults.
	UPROPERTY(EditDefaultsOnly, Category = "Solar System|UI")
	TSubclassOf<UUserWidget> TimeSliderWidgetClass;

	// All spawned planets — loop over this for "do X on every planet" logic.
	UFUNCTION(BlueprintPure, Category = "Solar System")
	const TArray<APlanetBase*>& GetPlanets() const { return Planets; }

private:
	astro_time_t BaseAstroTime;

	TWeakObjectPtr<UUserWidget> SliderWidget;

protected:
	virtual void BeginPlay() override;

	void CollectPlanets();
	void UpdatePositions(const astro_time_t& AstroTime);
	void UpdateBody(APlanetBase* Planet, const astro_time_t& AstroTime);

	static astro_body_t ToAstroBody(EPlanetBody Body);
	astro_time_t CurrentAstroTime();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Solar System")
	TArray<APlanetBase*> Planets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	float AUScale = 1000.0f;
};