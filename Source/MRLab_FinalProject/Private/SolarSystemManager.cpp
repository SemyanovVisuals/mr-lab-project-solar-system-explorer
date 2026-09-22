// Fill out your copyright notice in the Description page of Project Settings.


#include "SolarSystemManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

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

    UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: AUScale = %f, DistanceExponent = %f"), AUScale, DistanceExponent);

    // Capture the real-world time once at startup — this is the "today" baseline.
    // All slider offsets are relative to this moment.
    BaseAstroTime = CurrentAstroTime();

    // Show the solar system at today's positions immediately (no repeating timer needed).
    UpdatePositions(BaseAstroTime);

    // Create and show the time slider widget if a class has been assigned.
    if (TimeSliderWidgetClass)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            UUserWidget* Widget = CreateWidget<UUserWidget>(PC, TimeSliderWidgetClass);
            if (Widget)
            {
                Widget->AddToViewport();
                SliderWidget = Widget;
            }
        }
    }
}

void ASolarSystemManager::SetSimulatedOffsetYears(float OffsetYears)
{
    const double OffsetDays = static_cast<double>(OffsetYears) * 365.25;
    const astro_time_t SimulatedTime = Astronomy_AddDays(BaseAstroTime, OffsetDays);
    UpdatePositions(SimulatedTime);
}

void ASolarSystemManager::UpdatePositions(const astro_time_t& AstroTime)
{
    UpdateBody(mercuryMesh, BODY_MERCURY, AstroTime);
    UpdateBody(venusMesh,   BODY_VENUS,   AstroTime);
    UpdateBody(earthMesh,   BODY_EARTH,   AstroTime);
    UpdateBody(moonMesh,    BODY_MOON,    AstroTime);
    UpdateBody(marsMesh,    BODY_MARS,    AstroTime);
    UpdateBody(jupiterMesh, BODY_JUPITER, AstroTime);
    UpdateBody(saturnMesh,  BODY_SATURN,  AstroTime);
    UpdateBody(uranusMesh,  BODY_URANUS,  AstroTime);
    UpdateBody(neptuneMesh, BODY_NEPTUNE, AstroTime);
}

void ASolarSystemManager::UpdateBody(
    UStaticMeshComponent* Mesh,
    astro_body_t Body,
    const astro_time_t& AstroTime)
{
    if (!Mesh) return;

    const astro_vector_t Vec =
        Astronomy_HelioVector(Body, AstroTime);

    const double R_AU = FMath::Sqrt(Vec.x * Vec.x + Vec.y * Vec.y);
    const double CompressedFactor =
        (R_AU > UE_DOUBLE_SMALL_NUMBER)
            ? FMath::Pow(R_AU, DistanceExponent) / R_AU
            : 0.0;

    const FVector Position(
        Vec.x * CompressedFactor * AUScale,
        -Vec.y * CompressedFactor * AUScale,
        0.0
    );

    Mesh->SetRelativeLocation(Position);
}

void ASolarSystemManager::LoadMeshes()
{
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: GetComponents found %d StaticMeshComponent(s):"), Meshes.Num());
    for (UStaticMeshComponent* M : Meshes)
        UE_LOG(LogTemp, Warning, TEXT("  '%s'"), *M->GetName());

    // Use StartsWith so names like "Mercury_GEN_VARIABLE" or "MercuryMesh" still match.
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        const FString Name = Mesh->GetName();
        if      (Name.StartsWith(TEXT("Mercury"))) mercuryMesh = Mesh;
        else if (Name.StartsWith(TEXT("Venus")))   venusMesh   = Mesh;
        else if (Name.StartsWith(TEXT("Earth")))   earthMesh   = Mesh;
        else if (Name.StartsWith(TEXT("Moon")))    moonMesh    = Mesh;
        else if (Name.StartsWith(TEXT("Mars")))    marsMesh    = Mesh;
        else if (Name.StartsWith(TEXT("Jupiter"))) jupiterMesh = Mesh;
        else if (Name.StartsWith(TEXT("Saturn")))  saturnMesh  = Mesh;
        else if (Name.StartsWith(TEXT("Uranus")))  uranusMesh  = Mesh;
        else if (Name.StartsWith(TEXT("Neptune"))) neptuneMesh = Mesh;
    }

    auto Warn = [](const TCHAR* Body, const UStaticMeshComponent* Ptr)
    {
        if (!Ptr) UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: '%s' not matched — check component name in BP."), Body);
    };
    Warn(TEXT("Mercury"), mercuryMesh);
    Warn(TEXT("Venus"),   venusMesh);
    Warn(TEXT("Earth"),   earthMesh);
    Warn(TEXT("Moon"),    moonMesh);
    Warn(TEXT("Mars"),    marsMesh);
    Warn(TEXT("Jupiter"), jupiterMesh);
    Warn(TEXT("Saturn"),  saturnMesh);
    Warn(TEXT("Uranus"),  uranusMesh);
    Warn(TEXT("Neptune"), neptuneMesh);
}

astro_time_t ASolarSystemManager::CurrentAstroTime()
{
    FDateTime NowUtc = FDateTime::UtcNow();

    double Seconds =
        NowUtc.GetSecond() +
        NowUtc.GetMillisecond() / 1000.0;

    return Astronomy_MakeTime(
        NowUtc.GetYear(),
        NowUtc.GetMonth(),
        NowUtc.GetDay(),
        NowUtc.GetHour(),
        NowUtc.GetMinute(),
        Seconds
    );
}
