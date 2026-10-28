// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SharedMathLibrary.generated.h"

/**
 * 
 */
UCLASS()
class MRLAB_FINALPROJECT_API USharedMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
		UFUNCTION(BlueprintPure, Category = "Camera Math")
		static FQuat MakeRollFreeRigRotation(const FVector& LocalDir, const FVector& DesiredDir);
};
