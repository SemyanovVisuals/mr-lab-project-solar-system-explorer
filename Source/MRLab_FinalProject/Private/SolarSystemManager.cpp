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

	UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: BeginPlay"));

    LoadMeshes();

    // Update positions 4 times / second
    GetWorldTimerManager().SetTimer(
        UpdateTimer,
        this,
        &ASolarSystemManager::UpdatePositions,
        0.25f,
        true
    );
}

void ASolarSystemManager::UpdatePositions()
{
    astro_time_t AstroTime = CurrentAstroTime();

    UpdateBody(mercuryMesh, BODY_MERCURY, AstroTime);
    UpdateBody(venusMesh, BODY_VENUS, AstroTime);
    UpdateBody(earthMesh, BODY_EARTH, AstroTime);
    UpdateBody(moonMesh, BODY_MOON, AstroTime);
    UpdateBody(marsMesh, BODY_MARS, AstroTime);
    UpdateBody(jupiterMesh, BODY_JUPITER, AstroTime);
    UpdateBody(saturnMesh, BODY_SATURN, AstroTime);
    UpdateBody(uranusMesh, BODY_URANUS, AstroTime);
    UpdateBody(neptuneMesh, BODY_NEPTUNE, AstroTime);
}

void ASolarSystemManager::UpdateBody(
    UStaticMeshComponent* Mesh,
    astro_body_t Body,
    const astro_time_t& AstroTime)
{
    const astro_vector_t Vec =
        Astronomy_HelioVector(Body, AstroTime);

    const FVector Position(
        Vec.x * AUScale,
        -Vec.y * AUScale,
        0.0
    );

    Mesh->SetRelativeLocation(Position);
}

void ASolarSystemManager::LoadMeshes()
{
    mercuryMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Mercury")));
    venusMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Venus")));
    earthMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Earth")));
    moonMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Moon")));
    marsMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Mars")));
    jupiterMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Jupiter")));
    saturnMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Saturn")));
    uranusMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Uranus")));
    neptuneMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Neptune")));
}

astro_time_t ASolarSystemManager::CurrentAstroTime()
{
    FDateTime NowUtc = FDateTime::UtcNow();
    FString nowUtcString = NowUtc.ToString();

    //UE_LOG(LogTemp, Warning, TEXT("Current UTC: %s"), *nowUtcString);

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

