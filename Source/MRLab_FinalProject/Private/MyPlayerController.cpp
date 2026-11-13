#include "MyPlayerController.h"
#include "CinematicCameraComponent.h"
#include "PlanetBase.h"
#include "SharedMathLibrary.h"
#include "GameFramework/Pawn.h"
#include "SolarSystemManager.h"
#include "Kismet/GameplayStatics.h"

void AMyPlayerController::BeginPlay()
{
	// Before Super, so the Blueprint's BeginPlay event can already use them.
	CinematicComponent = FindComponentByClass<UCinematicCameraComponent>();
	SolarSystem = Cast<ASolarSystemManager>(
		UGameplayStatics::GetActorOfClass(this, ASolarSystemManager::StaticClass()));

	Super::BeginPlay();
}

// ---------- Highlight ----------

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHighlight();
}

void AMyPlayerController::UpdateHighlight()
{
	// During cinematics the view moves on its own: clear the highlight.
	if (!IsNotCinematic() || LockedPlanet)
	{
		SetHighlightedPlanet(nullptr);
		return;
	}

	APlanetBase* PlanetInView = SolarSystem
		? SolarSystem->FindPlanetInCone(GetEyeLocation(), GetScreenDirection(),
			static_cast<float>(ConeHalfAngle), static_cast<float>(TraceDistance))
		: nullptr;
	if (PlanetInView && PlanetInView->isSun())
	{
		return;
	}

	SetHighlightedPlanet(PlanetInView);
}

void AMyPlayerController::SetHighlightedPlanet(APlanetBase* NewPlanet)
{
	// Only act when the planet in view changes.
	if (NewPlanet == HighlightedPlanet)
	{
		return;
	}

	if (IsValid(HighlightedPlanet))
	{
		HighlightedPlanet->SetHighlighted(false);
	}

	if (IsValid(NewPlanet))
	{
		NewPlanet->SetHighlighted(true);
	}

	HighlightedPlanet = NewPlanet;
}

// ---------- Input entry point ----------

void AMyPlayerController::HandleMove(FVector2D Input, float ElapsedSeconds)
{
	if (!IsNotCinematic())
	{
		return; // no input during cinematics
	}

	if (bTargetLocked)
	{
		ZoomInOrbit(Input.Y);
		return;
	}

	if (bMovementLocked)
	{
		UE_LOG(LogTemp, Log, TEXT("MRPlayerController: Movement is locked"));
		return;
	}

	FlyForward(Input.Y, ElapsedSeconds);
}

// ---------- Fly mode ----------

void AMyPlayerController::FlyForward(float InputY, float ElapsedSeconds)
{
	APawn* Rig = GetPawn();
	if (!Rig) return;

	const double DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// Speed this frame, ramping up the longer the input is held.
	const double FlySpeed = GetScaledBaseFlySpeed() * GetFlyRampMultiplier(ElapsedSeconds) * DeltaSeconds * InputY;

	const double MaxStep = GetScaledMaxFlySpeed() * DeltaSeconds;
	const double ClampedFlySpeed = FMath::Clamp(FlySpeed, -MaxStep, MaxStep);

	const FVector Offset = GetScreenDirection() * ClampedFlySpeed;
	Rig->SetActorLocation(GetRigLocation() + Offset);
}

// ---------- Orbit zoom (eye-based) ----------

void AMyPlayerController::ZoomInOrbit(float InputY)
{
	APawn* Rig = GetPawn();
	if (!Rig || !LockedPlanet) return;

	// Measure from the eye, like the cinematic and the fill-distance calculation.
	const FVector Focus = GetFocusPoint();
	const FVector Eye = GetEyeLocation();
	const FVector Offset = Eye - Focus;
	const double Distance = Offset.Size();
	if (Distance < KINDA_SMALL_NUMBER) return;

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	// Exponential zoom: the step shrinks as the camera gets closer. Stick forward = zoom in.
	const double ZoomFactor = FMath::Exp(ZoomRate * DeltaSeconds * InputY * -1.0);
	const double NewDistance = FMath::Clamp(Distance * ZoomFactor, MinOrbitDistance, MaxOrbitDistance);

	// Where the eye should be, along the line from the planet through the eye.
	const FVector NewEye = Focus + (Offset / Distance) * NewDistance;

	// Move the rig by the same amount the eye needs to move.
	Rig->SetActorLocation(GetRigLocation() + (NewEye - Eye));
}

// ---------- Helpers ----------

FVector AMyPlayerController::GetRigLocation() const
{
	const APawn* Rig = GetPawn();
	return Rig ? Rig->GetActorLocation() : FVector::ZeroVector;
}

FVector AMyPlayerController::GetEyeLocation() const
{
	return ViewPoint ? ViewPoint->GetComponentLocation() : GetRigLocation();
}

FVector AMyPlayerController::GetScreenDirection() const
{
	if (ScreenCenter)
	{
		return (ScreenCenter->GetComponentLocation() - GetEyeLocation()).GetSafeNormal();
	}

	// Fallback without a screen reference: the rig's forward direction.
	const APawn* Rig = GetPawn();
	return Rig ? Rig->GetActorForwardVector() : FVector::ForwardVector;
}

FVector AMyPlayerController::GetFocusPoint() const
{
	return LockedPlanet ? LockedPlanet->GetActorLocation() : FVector::ZeroVector;
}

bool AMyPlayerController::IsNotCinematic() const
{
	return !CinematicComponent || !CinematicComponent->IsCinematicActive();
}

double AMyPlayerController::GetOrbitRadius() const
{
	return FVector::Dist(GetEyeLocation(), GetFocusPoint());
}

double AMyPlayerController::GetFlyRampMultiplier(double ElapsedSeconds) const
{
	return FMath::Clamp(ElapsedSeconds * FlyRampSpeed + FlyRampSpeed, 0.0, MaxFlyRampMultiplier);
}

// ---------- Camera look entry point ----------

void AMyPlayerController::HandleCameraLook(FVector2D Input, float ElapsedSeconds)
{
	if (!IsNotCinematic())
	{
		return; // no input during cinematics
	}

	if (bTargetLocked)
	{
		OrbitTarget(Input, ElapsedSeconds);
	}
	else
	{
		FreeLook(Input);
	}
}

// ---------- Free look ----------

void AMyPlayerController::FreeLook(const FVector2D& Input)
{
	APawn* Rig = GetPawn();
	if (!Rig) return;

	// Rates are in degrees per second, so the turning speed doesn't depend on frame rate.
	const double DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const double Yaw = Input.X * YawRate * DeltaSeconds;
	const double Pitch = Input.Y * -1.0 * PitchRate * DeltaSeconds;

	// FRotator takes (Pitch, Yaw, Roll).
	const FQuat YawQuat = FRotator(0.0, Yaw, 0.0).Quaternion();
	const FQuat PitchQuat = FRotator(Pitch, 0.0, 0.0).Quaternion();

	// Yaw on the left = around world up; pitch on the right = around the rig's own right axis.
	const FQuat NewQuat = YawQuat * Rig->GetActorQuat() * PitchQuat;
	Rig->SetActorRotation(NewQuat);
}

// ---------- Orbit (eye-based) ----------

void AMyPlayerController::OrbitTarget(const FVector2D& Input, float ElapsedSeconds)
{
	APawn* Rig = GetPawn();
	if (!Rig || !LockedPlanet) return;

	// Starting values, captured once (EyeStart / RigTransformStart in the Blueprint).
	const FVector Focus = GetFocusPoint();
	const FVector EyeStart = GetEyeLocation();
	const FTransform RigTransformStart = Rig->GetActorTransform();

	const FVector Offset = EyeStart - Focus;
	const double Radius = Offset.Size();
	if (Radius < KINDA_SMALL_NUMBER) return;

	// How far to orbit this frame, in degrees.
	const double DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const double Step = GetOrbitAngularRate() * GetOrbitRampMultiplier(ElapsedSeconds) * DeltaSeconds;
	const double AngleDeltaX = Input.X * Step;
	const double AngleDeltaY = Input.Y * Step;

	// Horizontal orbit around world up, then vertical around the horizontal axis perpendicular to the offset.
	const FVector OffsetH = Offset.RotateAngleAxis(AngleDeltaX, FVector::UpVector);
	const FVector VAxis = FVector::CrossProduct(OffsetH, FVector::UpVector).GetSafeNormal();
	const FVector OffsetV = OffsetH.RotateAngleAxis(AngleDeltaY, VAxis);

	// Pole guard: skip the vertical step when the eye would end up almost directly above or below.
	const bool bNearPole = FMath::Abs(OffsetV.GetSafeNormal() | FVector::UpVector) >= 0.99;
	const FVector NewOffset = bNearPole ? OffsetH : OffsetV;

	// New eye position, at the same distance from the planet.
	const FVector NewEye = Focus + NewOffset.GetSafeNormal() * Radius;

	// Rotate the rig by exactly the change in direction from the eye to the planet.
	const FQuat OldDir = FRotationMatrix::MakeFromX(Focus - EyeStart).ToQuat();
	const FQuat NewDir = FRotationMatrix::MakeFromX(Focus - NewEye).ToQuat();
	const FQuat DeltaQuat = NewDir * OldDir.Inverse();

	const FVector ToPlanetLocal = RigTransformStart.InverseTransformVector(Focus - EyeStart);
	const FQuat NewRigQuat = USharedMathLibrary::MakeRollFreeRigRotation(ToPlanetLocal, Focus - NewEye);

	// Place the rig so the eye lands exactly at NewEye with the new rotation.
	const FVector EyeLocal = RigTransformStart.InverseTransformPosition(EyeStart);
	const FVector NewRigLocation = NewEye - NewRigQuat.RotateVector(EyeLocal);

	Rig->SetActorLocationAndRotation(NewRigLocation, NewRigQuat);
}

// ---------- Orbit helpers ----------

double AMyPlayerController::GetOrbitAngularRate() const
{
	// Same as the Blueprint: 1 / (radius × 10), clamped to the min and max speed.
	const double Radius = GetOrbitRadius();
	const double RawRate = (Radius > KINDA_SMALL_NUMBER) ? 1.0 / (Radius * 10.0) : MaxAngularSpeed;
	return FMath::Clamp(RawRate, MinAngularSpeed, MaxAngularSpeed);
}

double AMyPlayerController::GetOrbitRampMultiplier(double ElapsedSeconds) const
{
	return FMath::Clamp(ElapsedSeconds * OrbitRampSpeed + OrbitRampSpeed, 0.0, MaxOrbitRampMultiplier);
}

// ---------- Target lock ----------

void AMyPlayerController::HandleTargetLock()
{
	if (!IsNotCinematic())
	{
		return; // no input during cinematics
	}

	if (bTargetLocked)
	{
		Unlock();
		return;
	}

	if (!IsValid(HighlightedPlanet))
	{
		UE_LOG(LogTemp, Log, TEXT("MyPlayerController: No target hit"));
		return;
	}

	LockOnto(HighlightedPlanet);
}

void AMyPlayerController::LockOnto(APlanetBase* Planet)
{
	LockedPlanet = Planet;
	bTargetLocked = true;

	// Minimum distance based on the planet's size and the screen geometry.
	const float FillDistance = Planet->GetViewDistanceForFill(
		static_cast<float>(ZoomFill),
		static_cast<float>(ScreenHalfHeight),
		static_cast<float>(GetScreenDistance()));

	// Keep the previous value if the inputs were invalid (the function returns 0 then).
	if (FillDistance > 0.0f)
	{
		MinOrbitDistance = FillDistance;
	}

	if (CinematicComponent)
	{
		const double PlanetScreenRadius = ZoomFill * ScreenHalfHeight;
		const FVector2D CornerOffset(
			-FMath::Max(0.0, ScreenHalfWidth - PlanetScreenRadius - CornerMargin),
			-FMath::Max(0.0, ScreenHalfHeight - PlanetScreenRadius - CornerMargin));
		CinematicComponent->StartAutoZoom(Planet, static_cast<float>(MinOrbitDistance), CornerOffset);
	}
}

void AMyPlayerController::Unlock()
{
	bTargetLocked = false;
	LockedPlanet = nullptr;
}

double AMyPlayerController::GetScreenDistance() const
{
	return ScreenCenter
		? FVector::Dist(ScreenCenter->GetComponentLocation(), GetEyeLocation())
		: 0.0;
}

// ---------- Movement lock ----------

void AMyPlayerController::HandleMovementLock()
{
	bMovementLocked = !bMovementLocked;
}

// ---------- Quiz Logic -------------

void AMyPlayerController::HandleQuizAbandoned()
{
	if (CinematicComponent && LockedPlanet) {
		CinematicComponent->StartRecenter(LockedPlanet);
	}
}

// ---------- Time scroll ------------

void AMyPlayerController::HandleTimeScroll(float Direction, float ElapsedSeconds)
{
	if (LockedPlanet || !SolarSystem)
	{
		return; // no input during cinematics
	}

	if (bTimeScrollSuppressed)
	{
		return; // just jumped to "now": wait until both buttons are released
	}

	// Exponential ramp: the range spans days to decades, so the speed doubles while held.
	const double Rate = FMath::Min(
		TimeScrollStartRate * FMath::Pow(2.0, ElapsedSeconds / TimeScrollDoublingTime),
		TimeScrollMaxRate);

	const double Step = FMath::Sign(Direction) * Rate * GetWorld()->GetDeltaSeconds();
	SolarSystem->SetSimulatedOffsetYears(SolarSystem->GetSimulatedOffsetYears() + Step);
}

void AMyPlayerController::SetTimeButtonHeld(float Direction, bool bHeld)
{
	(Direction < 0.0f ? bTimeBackHeld : bTimeFwdHeld) = bHeld;

	if (bTimeBackHeld && bTimeFwdHeld)
	{
		// Once, when the second button goes down.
		ResetTimeToNow();

		// Otherwise the button still held would keep scrolling at its ramped-up speed.
		bTimeScrollSuppressed = true;
	}
	else if (!bTimeBackHeld && !bTimeFwdHeld)
	{
		bTimeScrollSuppressed = false;
	}
}

void AMyPlayerController::ResetTimeToNow()
{
	if (LockedPlanet || !SolarSystem)
	{
		return; // same rule as scrolling
	}

	SolarSystem->SetSimulatedOffsetYears(0.0);
}

// ---------- Setup ------------------

void AMyPlayerController::SetNDisplayReferences(USceneComponent* InEye,
	USceneComponent* InScreenCenter, USceneComponent* InScreen)
{
	ViewPoint = InEye;
	ScreenCenter = InScreenCenter;

	// The screen's size is stored in its scale (in cm): half its height is half the Z scale.
	if (InScreen)
	{
		const FVector ScreenScale = InScreen->GetComponentScale();
		ScreenHalfHeight = ScreenScale.Z * 0.5;
		ScreenHalfWidth = ScreenScale.Y * 0.5;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPlayerController: no screen component passed, keeping Screen Half Height = %.1f"),
			ScreenHalfHeight);
	}

	if (!ViewPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPlayerController: no eye component passed."));
	}
	if (!ScreenCenter)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPlayerController: no screen center component passed."));
	}

	// The cinematic uses the same eye as the controller.
	if (CinematicComponent)
	{
		CinematicComponent->ViewPoint = ViewPoint;
		CinematicComponent->ScreenCenter = ScreenCenter;
	}

	if (const APawn* Rig = GetPawn())
	{
		const FTransform RigTransform = Rig->GetActorTransform();
	}
}