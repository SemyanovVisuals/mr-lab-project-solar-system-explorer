// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
extern "C"
{
#include "../ExternalScripts/astronomy.h"
}
#include "SolarSystemManager.generated.h"

UCLASS()
class MRLAB_FINALPROJECT_API ASolarSystemManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASolarSystemManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	astro_time_t CurrentAstroTime();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	UStaticMeshComponent* EarthMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar System")
	float AUScale = 1000.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
