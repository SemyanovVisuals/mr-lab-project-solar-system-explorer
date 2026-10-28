// Fill out your copyright notice in the Description page of Project Settings.


#include "SharedMathLibrary.h"


FQuat USharedMathLibrary::MakeRollFreeRigRotation(const FVector& LocalDir, const FVector& DesiredDir)
{
	const FVector D = LocalDir.GetSafeNormal();
	const FRotator Desired = DesiredDir.GetSafeNormal().Rotation();
	const double DesiredPitch = FMath::DegreesToRadians(Desired.Pitch);
	const double DesiredYaw = FMath::DegreesToRadians(Desired.Yaw);

	const double A = FMath::Sqrt(D.X * D.X + D.Z * D.Z);
	const double Alpha = FMath::Atan2(D.Z, D.X);
	const double S = (A > KINDA_SMALL_NUMBER) ? FMath::Clamp(FMath::Sin(DesiredPitch) / A, -1.0, 1.0) : 0.0;
	const double RigPitch = FMath::Asin(S) - Alpha;

	const double PitchedX = D.X * FMath::Cos(RigPitch) - D.Z * FMath::Sin(RigPitch);
	const double RigYaw = DesiredYaw - FMath::Atan2(D.Y, PitchedX);

	return FRotator(FMath::RadiansToDegrees(RigPitch), FMath::RadiansToDegrees(RigYaw), 0.0).Quaternion();
}