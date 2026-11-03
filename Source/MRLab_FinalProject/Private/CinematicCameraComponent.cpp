// Fill out your copyright notice in the Description page of Project Settings.


#include "CinematicCameraComponent.h"
#include "PlanetBase.h"
#include "SharedMathLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// 0 at the start and end of the fly-by, 1 in the middle, with smooth transitions.
	double Envelope(double Alpha, double Fraction)
	{
		const double In = FMath::Clamp(Alpha / Fraction, 0.0, 1.0);
		const double Out = FMath::Clamp((1.0 - Alpha) / Fraction, 0.0, 1.0);
		return FMath::SmoothStep(0.0, 1.0, FMath::Min(In, Out));
	}
}

// Sets default values for this component's properties
UCinematicCameraComponent::UCinematicCameraComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // only ticks during a cinematic
	// ...
}


// Called when the game starts
void UCinematicCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCinematicCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlanetBase* Planet = TargetPlanet.Get();
	if (Mode == EMode::None || !Planet || !GetControlledPawn())
	{
		Finish();
		return;
	}

	Elapsed += DeltaTime;
	const FVector Center = Planet->GetActorLocation(); // follows the planet if it moves

	if (Mode == EMode::AutoZoom || Mode == EMode::Recenter)
	{
		const float Duration = (Mode == EMode::Recenter) ? AlignDuration : AutoZoomDuration;
		const double Alpha = FMath::Clamp(Elapsed / FMath::Max(KINDA_SMALL_NUMBER, Duration), 0.0f, 1.0f);
		const double Eased = FMath::InterpEaseInOut(0.0, 1.0, static_cast<float>(Alpha), 2.0f);

		ApplyCamera(Center, FMath::Lerp(StartDistance, EndDistance, Eased),
			StartAzimuth, StartElevation, 0.0, 0.0);

		if (Alpha >= 1.0 && Elapsed >= AlignDuration) Finish();
	}
	else // FlyBy
	{
		const double Alpha = FMath::Clamp(Elapsed / FlyByDuration, 0.0f, 1.0f);
		const double Env = Envelope(Alpha, BlendFraction);
		const double Sign = bReverseDirection ? -1.0 : 1.0;

		// Full circle, easing in at the start and out at the end.
		const double Azimuth = StartAzimuth +
			Sign * 360.0 * FMath::InterpEaseInOut(0.0, 1.0, static_cast<float>(Alpha), 2.0f);

		// Start distance -> fly-by distance -> return distance.
		const double BaseDistance = FMath::Lerp(StartDistance, EndDistance, Alpha);
		const double Distance = FMath::Lerp(BaseDistance, FlyByDistance, Env);

		const double Elevation = FMath::Lerp(StartElevation, static_cast<double>(FlyByElevation), Env);

		ApplyCamera(Center, Distance, Azimuth, Elevation, LookAheadAngle * Env, Env);

		if (Alpha >= 1.0) Finish();
	}
}


APawn* UCinematicCameraComponent::GetControlledPawn() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC ? PC->GetPawn() : nullptr;
}

bool UCinematicCameraComponent::BeginCinematic(APlanetBase* Planet, EMode NewMode)
{
	APawn* Pawn = GetControlledPawn();
	if (!Planet || !Pawn) return false;

	// Eye position inside the rig, captured once so head movement still gives parallax.
	EyeLocalOffset = ViewPoint
		? Pawn->GetActorTransform().InverseTransformPosition(ViewPoint->GetComponentLocation())
		: FVector::ZeroVector;

	ViewLocalRotation = FQuat::Identity;
	if (ScreenCenter)
	{
		const FVector AimWorld = ScreenCenter->GetComponentLocation()
			+ ScreenCenter->GetRightVector() * AimOffset.X
			+ ScreenCenter->GetUpVector() * AimOffset.Y;

		const FVector AimLocal = Pawn->GetActorTransform().InverseTransformPosition(AimWorld);
		const FVector ViewDirLocal = (AimLocal - EyeLocalOffset).GetSafeNormal();
		if (!ViewDirLocal.IsNearlyZero())
		{
			ViewLocalRotation = FRotationMatrix::MakeFromX(ViewDirLocal).ToQuat();
		}
	}

	const FVector EyeLocation = Pawn->GetActorTransform().TransformPosition(EyeLocalOffset);
	const FVector Offset = EyeLocation - Planet->GetActorLocation();

	StartDistance = Offset.Size();
	if (StartDistance < KINDA_SMALL_NUMBER) return false;

	// Fix the orbit basis for the whole cinematic.
	GetOrbitBasis(BasisU, BasisV, BasisAxis);
	StartRigRotation = Pawn->GetActorQuat();

	StartAzimuth = FMath::RadiansToDegrees(FMath::Atan2(Offset | BasisV, Offset | BasisU));
	StartElevation = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp((Offset | BasisAxis) / StartDistance, -1.0, 1.0)));

	// Choose the direction of the camera's right vector so the camera starts upright.
	const FVector StartForward = -Offset.GetSafeNormal();
	RightSign = ((StartForward ^ BasisAxis).Z >= 0.0) ? 1.0 : -1.0;

	TargetPlanet = Planet;
	Mode = NewMode;
	Elapsed = 0.0f;
	SetComponentTickEnabled(true);
	return true;
}

void UCinematicCameraComponent::StartAutoZoom(APlanetBase* Planet, float TargetDistance, FVector2D ScreenAimOffset)
{
	AimOffset = ScreenAimOffset;
	if (BeginCinematic(Planet, EMode::AutoZoom))
	{
		EndDistance = TargetDistance;
	}
}

void UCinematicCameraComponent::StartFlyBy(APlanetBase* Planet, float ReturnDistance)
{
	AimOffset = FVector2D::ZeroVector;
	if (BeginCinematic(Planet, EMode::FlyBy))
	{
		EndDistance = ReturnDistance;

		// For a sphere mesh, the bounds' box extent equals its (scaled) radius.
		const double PlanetRadius = Planet->Mesh ? Planet->Mesh->Bounds.BoxExtent.GetMax() : 0.0;
		FlyByDistance = PlanetRadius > 0.0 ? PlanetRadius * FlyByRadiusFactor : StartDistance;
	}
}

void UCinematicCameraComponent::ApplyCamera(const FVector& Center, double Distance,
	double AzimuthDeg, double ElevationDeg, double LookAheadDeg, double FrameBlend)
{
	APawn* Pawn = GetControlledPawn();
	if (!Pawn) return;

	const double Az = FMath::DegreesToRadians(AzimuthDeg);
	const double El = FMath::DegreesToRadians(ElevationDeg);

	// Position on the orbit, in the chosen basis.
	const FVector Radial =
		FMath::Cos(El) * (FMath::Cos(Az) * BasisU + FMath::Sin(Az) * BasisV) +
		FMath::Sin(El) * BasisAxis;
	const FVector NewLocation = Center + Radial * Distance;

	// Look at the planet, then turn toward the direction of travel.
	const double Sign = bReverseDirection ? -1.0 : 1.0;
	const FVector ToCenter = -Radial;
	const FVector Tangent = Sign * (-FMath::Sin(Az) * BasisU + FMath::Cos(Az) * BasisV);
	const double LookAhead = FMath::DegreesToRadians(LookAheadDeg);
	const FVector Forward = (FMath::Cos(LookAhead) * ToCenter + FMath::Sin(LookAhead) * Tangent).GetSafeNormal();

	// Upright frame (world up) at the start/end; orbit-aligned frame during the fly-by.
	const FQuat WorldFrame = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();
	const FQuat OrbitFrame = (OrbitAxis == EOrbitAxis::Z)
		? WorldFrame
		: FRotationMatrix::MakeFromXY(Forward, RightSign * BasisAxis).ToQuat();
	const FQuat NewQuat = FQuat::Slerp(WorldFrame, OrbitFrame, FMath::Clamp(FrameBlend, 0.0, 1.0));

	const FQuat RollFreeRig = USharedMathLibrary::MakeRollFreeRigRotation(ViewLocalRotation.GetForwardVector(), NewQuat.GetForwardVector());

	const FQuat TargetRigQuat = (OrbitAxis == EOrbitAxis::Z)
		? RollFreeRig
		: FQuat::Slerp(RollFreeRig, NewQuat * ViewLocalRotation.Inverse(), FMath::Clamp(FrameBlend, 0.0, 1.0));

	// Turn smoothly from the rotation at the start of the cinematic, instead of snapping.
	const float AlignAlpha = (AlignDuration > 0.0f) ? FMath::Clamp(Elapsed / AlignDuration, 0.0f, 1.0f) : 1.0f;
	const float AlignEased = FMath::InterpEaseInOut(0.0f, 1.0f, AlignAlpha, 2.0f);

	const FRotator StartRot = StartRigRotation.Rotator();
	const FRotator TargetRot = TargetRigQuat.Rotator();
	const FRotator FinalRot = StartRot + (TargetRot - StartRot).GetNormalized() * AlignEased;
	const FQuat FinalQuat = FinalRot.Quaternion();

	// NewLocation is where the eye should be; place the rig so the eye lands there.
	const FTransform NewRigTransform(FinalQuat, FVector::ZeroVector, Pawn->GetActorScale3D());
	const FVector NewRigLocation = NewLocation - NewRigTransform.TransformVector(EyeLocalOffset);

	Pawn->SetActorLocationAndRotation(NewRigLocation, FinalQuat);
}

void UCinematicCameraComponent::Finish()
{
	const EMode FinishedMode = Mode;

	Mode = EMode::None;
	TargetPlanet.Reset();
	SetComponentTickEnabled(false);

	if (FinishedMode == EMode::AutoZoom)    OnAutoZoomFinished.Broadcast();
	else if (FinishedMode == EMode::FlyBy)  OnFlyByFinished.Broadcast();
	else if (FinishedMode == EMode::Recenter) OnRecenterFinished.Broadcast();
}

void UCinematicCameraComponent::GetOrbitBasis(FVector& OutU, FVector& OutV, FVector& OutAxis) const
{
	switch (OrbitAxis)
	{
	case EOrbitAxis::X:
		OutU = FVector::YAxisVector; OutV = FVector::ZAxisVector; OutAxis = FVector::XAxisVector;
		break;
	case EOrbitAxis::Y:
		OutU = FVector::ZAxisVector; OutV = FVector::XAxisVector; OutAxis = FVector::YAxisVector;
		break;
	default: // Z
		OutU = FVector::XAxisVector; OutV = FVector::YAxisVector; OutAxis = FVector::ZAxisVector;
		break;
	}
}

void UCinematicCameraComponent::StartRecenter(APlanetBase* Planet)
{
	AimOffset = FVector2D::ZeroVector;
	if (BeginCinematic(Planet, EMode::Recenter))
	{
		EndDistance = StartDistance;
	}
}