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
class UUserWidget;

UCLASS()
class MRLAB_FINALPROJECT_API ASolarSystemManager : public AActor
{
	GENERATED_BODY()

public:
	ASolarSystemManager();

	// Limit of the simulated time offset, in years either side of today.
	static constexpr double MaxOffsetYears = 50.0;

	// OffsetYears: 0 = today, negative = past, positive = future. Clamped to ±MaxOffsetYears.
	UFUNCTION(BlueprintCallable, Category = "Solar System")
	void SetSimulatedOffsetYears(double OffsetYears);

	UFUNCTION(BlueprintPure, Category = "Solar System")
	double GetSimulatedOffsetYears() const { return CurrentOffsetYears; }

	// Widget Blueprint class to instantiate. Set this in BP_SolarSystemManager class defaults.
	UPROPERTY(EditDefaultsOnly, Category = "Solar System|UI")
	TSubclassOf<UUserWidget> TimeSliderWidgetClass;

	// All spawned planets � loop over this for "do X on every planet" logic.
	UFUNCTION(BlueprintPure, Category = "Solar System")
	const TArray<APlanetBase*>& GetPlanets() const { return Planets; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintPure = false), Category = "Solar System")
	APlanetBase* FindPlanetInCone(FVector Origin, FVector Direction,
	float ConeHalfAngleDeg, float MaxDistance) const;

#if WITH_EDITOR
	// Generates /Game/Materials/M_NeonArrows from Shaders/NeonArrows.hlsl and sorts the VisualizationPlane
	// below the arrows. Press again after editing the shader file, then save the level.
	UFUNCTION(CallInEditor, Category = "Solar System|Arrows")
	void BuildArrowAssets();
#endif

private:
	astro_time_t BaseAstroTime;

	// Double: float years are only accurate to ~2 minutes at the ±50 yr ends.
	double CurrentOffsetYears = 0.0;

	TWeakObjectPtr<UUserWidget> SliderWidget;

	// Resolved by name from the Blueprint's components in LoadMeshes(); not exposed in Details.
	UStaticMeshComponent* sunMesh     = nullptr;
	UStaticMeshComponent* mercuryMesh = nullptr;
	UStaticMeshComponent* venusMesh   = nullptr;
	UStaticMeshComponent* earthMesh   = nullptr;
	UStaticMeshComponent* moonMesh    = nullptr;
	UStaticMeshComponent* marsMesh    = nullptr;
	UStaticMeshComponent* jupiterMesh = nullptr;
	UStaticMeshComponent* saturnMesh  = nullptr;
	UStaticMeshComponent* uranusMesh  = nullptr;
	UStaticMeshComponent* neptuneMesh = nullptr;

protected:
	virtual void BeginPlay() override;

	void CollectPlanets();
	void AdoptPlanetDecorations(APlanetBase* Planet);
	void UpdatePositions(const astro_time_t& AstroTime);
	void UpdateBody(APlanetBase* Planet, const astro_time_t& AstroTime);

	static astro_body_t ToAstroBody(EPlanetBody Body);
	astro_time_t CurrentAstroTime();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Solar System")
	TArray<APlanetBase*> Planets;

	UPROPERTY()
	APlanetBase* SunPlanet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	float AUScale = 1000.0f;

	// Radius compression exponent applied before AUScale: r' = r^DistanceExponent.
	// 1.0 = true linear distances (original behavior). Lower values pull outer
	// planets in much more aggressively than inner ones, keeping angle/phase exact.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DistanceExponent = 0.4f;

	// Radius compression exponent applied relative to the Sun: visualScale = sunScale * (realRadius/sunRadius)^RadiusExponent.
	// 1.0 = true linear real-world radii (Sun ~109x Earth). Lower values compress the
	// range so small bodies stay visible next to the Sun while preserving ordering.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float RadiusExponent = 0.35f;
};
