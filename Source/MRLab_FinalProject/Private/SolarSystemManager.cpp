// Fill out your copyright notice in the Description page of Project Settings.


#include "SolarSystemManager.h"

extern "C"
{
#include "../ExternalScripts/astronomy.h"
}

// Sets default values
ASolarSystemManager::ASolarSystemManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASolarSystemManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager BeginPlay called"));

	const char* EarthName = Astronomy_BodyName(BODY_EARTH);
	double EarthPeriod = Astronomy_PlanetOrbitalPeriod(BODY_EARTH);

	UE_LOG(LogTemp, Warning, TEXT("Astronomy C test: body = %hs"), EarthName);
	UE_LOG(LogTemp, Warning, TEXT("Earth orbital period = %f days"), EarthPeriod);
	
}

// Called every frame
void ASolarSystemManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

