#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SolarSystemManager.h"
#include "MyPlayerController.generated.h"

class APlanetBase;
class UCinematicCameraComponent;
class ASolarSystemManager;

UCLASS()
class MRLAB_FINALPROJECT_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	// Called from the IA_Move event (Action Value, Elapsed Seconds).
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void HandleMove(FVector2D Input, float ElapsedSeconds);

	// Called from the IA_CameraLook event (Action Value, Elapsed Seconds).
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void HandleCameraLook(FVector2D Input, float ElapsedSeconds);

	// Called from the IA_TargetLock event.
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void HandleTargetLock();

	// Called from the IA_MovementLock event: toggles Movement Locked.
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void HandleMovementLock();

	// Called from BeginPlay in Blueprint with the nDisplay components.
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetNDisplayReferences(USceneComponent* InEye, USceneComponent* InScreenCenter, USceneComponent* InScreen);

	// ---------- References (set from Blueprint in BeginPlay) ----------

	// The tracked eye: NDC DefaultViewPoint.
	UPROPERTY(BlueprintReadWrite, Category = "References")
	USceneComponent* ViewPoint = nullptr;

	// Center of the screen: the nDisplay Screen component (or Screen Transform marker).
	UPROPERTY(BlueprintReadWrite, Category = "References")
	USceneComponent* ScreenCenter = nullptr;

	// Found automatically in BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "References")
	UCinematicCameraComponent* CinematicComponent = nullptr;

	// Found automatically in BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "References")
	ASolarSystemManager* SolarSystem = nullptr;

	// ---------- State ----------

	UPROPERTY(BlueprintReadWrite, Category = "State")
	bool bTargetLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "State")
	bool bMovementLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "State")
	APlanetBase* LockedPlanet = nullptr;

	// The planet currently in the selection cone (set by the highlight logic).
	UPROPERTY(BlueprintReadWrite, Category = "State")
	APlanetBase* HighlightedPlanet = nullptr;

	// ---------- Fly settings ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fly")
	double BaseFlySpeed = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fly")
	double MaxFlySpeed = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fly")
	double Scale = 25.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fly")
	double FlyRampSpeed = 2.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fly")
	double MaxFlyRampMultiplier = 10.0;

	// ---------- Orbit settings ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double ZoomRate = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double MinOrbitDistance = 500.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double MaxOrbitDistance = 5000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double MinAngularSpeed = 5.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double MaxAngularSpeed = 90.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double OrbitRampSpeed = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Orbit")
	double MaxOrbitRampMultiplier = 3.0;

	// ---------- Free look settings ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Free Look")
	double YawRate = 120.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Free Look")
	double PitchRate = 120.0;

	// ---------- Targeting settings ----------

	// How much of the screen height the planet fills at the minimum zoom distance (0–1).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	double ZoomFill = 0.8;

	// Half the physical screen height, set in BeginPlay from the nDisplay screen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	double ScreenHalfHeight = 150.0;

	// Half-angle of the selection cone, in degrees.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	double ConeHalfAngle = 5.0;

	// Planets further away than this can't be selected.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	double TraceDistance = 100000.0;

	// ---------- Helpers (replacing the Blueprint macros) ----------

	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetRigLocation() const;

	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetEyeLocation() const;

	// From the eye through the center of the screen.
	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetScreenDirection() const;

	// Location of the locked planet.
	UFUNCTION(BlueprintPure, Category = "Camera")
	FVector GetFocusPoint() const;

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsNotCinematic() const;

	// Distance from the rig to the focus point.
	UFUNCTION(BlueprintPure, Category = "Camera")
	double GetOrbitRadius() const;

	// Distance from the eye to the center of the screen.
	UFUNCTION(BlueprintPure, Category = "Camera")
	double GetScreenDistance() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Fly")
	double GetScaledBaseFlySpeed() const { return BaseFlySpeed * Scale; }

	UFUNCTION(BlueprintPure, Category = "Movement|Fly")
	double GetScaledMaxFlySpeed() const { return MaxFlySpeed * Scale; }

	UFUNCTION(BlueprintPure, Category = "Movement|Fly")
	double GetFlyRampMultiplier(double ElapsedSeconds) const;

	// Orbit speed in degrees per second, based on the distance to the planet.
	UFUNCTION(BlueprintPure, Category = "Movement|Orbit")
	double GetOrbitAngularRate() const;

	// Grows with how long the input has been held, capped at MaxOrbitRampMultiplier.
	UFUNCTION(BlueprintPure, Category = "Movement|Orbit")
	double GetOrbitRampMultiplier(double ElapsedSeconds) const;

protected:
	virtual void BeginPlay() override;

	void FlyForward(float InputY, float ElapsedSeconds);
	void ZoomInOrbit(float InputY);
	void FreeLook(const FVector2D& Input);
	void OrbitTarget(const FVector2D& Input, float ElapsedSeconds);
	void LockOnto(APlanetBase* Planet);
	void Unlock();
	void UpdateHighlight();
	void SetHighlightedPlanet(APlanetBase* NewPlanet);
};