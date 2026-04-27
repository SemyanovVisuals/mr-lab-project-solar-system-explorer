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

    // TODO: extract real time [year, month, day, hour, minute, second]
    //astro_time_t Time = Astronomy_MakeTime(2026, 4, 27, 16, 33, 42.706);

    UStaticMeshComponent* earthMesh =
        Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Earth")));

    astro_time_t AstroTime = CurrentAstroTime();

    astro_vector_t Earth = Astronomy_HelioVector(BODY_EARTH, AstroTime); // vector in AU

    if (Earth.status == ASTRO_SUCCESS)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Earth heliocentric position: x=%f AU, y=%f AU, z=%f AU"),
            Earth.x,
            Earth.y,
            Earth.z
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to calculate Earth position"));
    }

    if (earthMesh)
    {
        FVector earthPosition = FVector(
            Earth.x * AUScale,
            Earth.y * AUScale,
            0.0f
        );

        earthMesh->SetRelativeLocation(earthPosition);

        UE_LOG(LogTemp, Warning, TEXT("Earth moved to: %s"), *earthPosition.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find Earth mesh component"));
    }
	
}

astro_time_t ASolarSystemManager::CurrentAstroTime()
{
    FDateTime NowUtc = FDateTime::UtcNow();

    FDateTime nowUtc = FDateTime::UtcNow();
    FString nowUtcString = nowUtc.ToString();

    UE_LOG(LogTemp, Warning, TEXT("Current UTC: %s"), *nowUtcString);

    double Seconds =
        NowUtc.GetSecond() +
        NowUtc.GetMillisecond() / 1000.0;

    astro_time_t NowAstro = Astronomy_MakeTime(
        NowUtc.GetYear(),
        NowUtc.GetMonth(),
        NowUtc.GetDay(),
        NowUtc.GetHour(),
        NowUtc.GetMinute(),
        Seconds
    );

    return NowAstro;
}

// Called every frame
void ASolarSystemManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

