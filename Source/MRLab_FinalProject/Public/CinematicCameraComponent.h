// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CinematicCameraComponent.generated.h"

class APlanetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicFinished);

UENUM(BlueprintType)
enum class EOrbitAxis : uint8
{
	X UMETA(DisplayName = "X (orbit in YZ plane)"),
	Y UMETA(DisplayName = "Y (orbit in XZ plane)"),
	Z UMETA(DisplayName = "Z (orbit in XY plane)")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MRLAB_FINALPROJECT_API UCinematicCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCinematicCameraComponent();

	// Smoothly moves the camera towards the planet until TargetDistance (from the planet's center).
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void StartAutoZoom(APlanetBase* Planet, float TargetDistance);

	// Full fly-by: approach, 360° orbit with look-ahead, then end at ReturnDistance.
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void StartFlyBy(APlanetBase* Planet, float ReturnDistance);

	UFUNCTION(BlueprintPure, Category = "Cinematic")
	bool IsCinematicActive() const { return Mode != EMode::None; }

	UPROPERTY(BlueprintAssignable, Category = "Cinematic")
	FOnCinematicFinished OnAutoZoomFinished;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic")
	FOnCinematicFinished OnFlyByFinished;

	// --- Auto zoom settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Auto Zoom", meta = (ClampMin = "0.1"))
	float AutoZoomDuration = 2.5f;

	// --- Fly-by settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By", meta = (ClampMin = "1.0"))
	float FlyByDuration = 25.0f;

	// Fly-by distance from the planet's center, as a multiple of the planet's radius.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By", meta = (ClampMin = "1.05"))
	float FlyByRadiusFactor = 1.6f;

	// Height of the fly-by orbit above the planet's equator, in degrees.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By")
	float FlyByElevation = 10.0f;

	// How far the view turns away from the planet's center, toward the direction of travel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By")
	float LookAheadAngle = 45.0f;

	// Fraction of the fly-by spent approaching (and, at the end, returning).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By", meta = (ClampMin = "0.05", ClampMax = "0.45"))
	float BlendFraction = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By")
	bool bReverseDirection = false;

	// The tracked view point (NDC DefaultViewPoint). If not set, the pawn's origin is used.
	UPROPERTY(BlueprintReadWrite, Category = "Cinematic")
	USceneComponent* ViewPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Fly-By")
	EOrbitAxis OrbitAxis = EOrbitAxis::Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic", meta = (ClampMin = "0.0"))
	float AlignDuration = 1.5f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:	
	enum class EMode : uint8 { None, AutoZoom, FlyBy };

	EMode Mode = EMode::None;
	TWeakObjectPtr<APlanetBase> TargetPlanet;
	float Elapsed = 0.0f;

	double StartDistance = 0.0;
	double EndDistance = 0.0;
	double StartAzimuth = 0.0;
	double StartElevation = 0.0;
	double FlyByDistance = 0.0;
	FVector BasisU = FVector::XAxisVector;
	FVector BasisV = FVector::YAxisVector;
	FVector BasisAxis = FVector::ZAxisVector;
	double RightSign = 1.0;
	FQuat StartRigRotation = FQuat::Identity;

	FVector EyeLocalOffset = FVector::ZeroVector;

	APawn* GetControlledPawn() const;
	bool BeginCinematic(APlanetBase* Planet, EMode NewMode);
	void ApplyCamera(const FVector& Center, double Distance, double AzimuthDeg,
		double ElevationDeg, double LookAheadDeg, double FrameBlend);
	void Finish();
	void GetOrbitBasis(FVector& OutU, FVector& OutV, FVector& OutAxis) const;
};
