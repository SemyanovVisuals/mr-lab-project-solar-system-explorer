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

private:

	FTimerHandle UpdateTimer;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdatePositions();

	void LoadMeshes();

	void UpdateBody(
		UStaticMeshComponent* Mesh,
		astro_body_t Body,
		const astro_time_t& AstroTime
	);

	astro_time_t CurrentAstroTime();

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
};
