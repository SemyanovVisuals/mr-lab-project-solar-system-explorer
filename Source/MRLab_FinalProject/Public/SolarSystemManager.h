// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	// Sets default values for this actor's properties
	ASolarSystemManager();

	// Called by the time slider widget when the user drags the slider.
	// OffsetYears: 0 = today, -100 = 100 years in the past, +100 = 100 years in the future.
	UFUNCTION(BlueprintCallable, Category = "Solar System")
	void SetSimulatedOffsetYears(float OffsetYears);

	// Widget Blueprint class to instantiate. Set this in BP_SolarSystemManager class defaults.
	UPROPERTY(EditDefaultsOnly, Category = "Solar System|UI")
	TSubclassOf<UUserWidget> TimeSliderWidgetClass;

private:

	astro_time_t BaseAstroTime;

	TWeakObjectPtr<UUserWidget> SliderWidget;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdatePositions(const astro_time_t& AstroTime);

	void LoadMeshes();

	void UpdateBody(
		UStaticMeshComponent* Mesh,
		astro_body_t Body,
		const astro_time_t& AstroTime
	);

	astro_time_t CurrentAstroTime();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* sunMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* mercuryMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* venusMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* earthMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* moonMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* marsMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* jupiterMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* saturnMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* uranusMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* neptuneMesh = nullptr;

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
