// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSystemManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASolarSystemManager::ASolarSystemManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void ASolarSystemManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: BeginPlay"));


	CollectPlanets();

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

void ASolarSystemManager::CollectPlanets()
{
	Planets.Reset();

	TArray<AActor*> Attached;
	GetAttachedActors(Attached, true, false); // direct children only

	for (AActor* Actor : Attached)
	{
		if (APlanetBase* Planet = Cast<APlanetBase>(Actor))
		{
			Planets.Add(Planet);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: found %d planet(s)."), Planets.Num());
}

void ASolarSystemManager::SetSimulatedOffsetYears(float OffsetYears)
{
	const double OffsetDays = static_cast<double>(OffsetYears) * 365.25;
    const astro_time_t SimulatedTime = Astronomy_AddDays(BaseAstroTime, OffsetDays);
    UpdatePositions(SimulatedTime);
}

void ASolarSystemManager::UpdatePositions(const astro_time_t& AstroTime)
{
	for (APlanetBase* Planet : Planets)
	{
		if (Planet) UpdateBody(Planet, AstroTime);
	}
}

void ASolarSystemManager::UpdateBody(APlanetBase* Planet, const astro_time_t& AstroTime)
{
	const astro_vector_t Vec = Astronomy_HelioVector(ToAstroBody(Planet->Body), AstroTime);
	if (Vec.status != ASTRO_SUCCESS) return;

	const FVector Position(
		Vec.x * AUScale,
		-Vec.y * AUScale,
		0.0
	);

	Planet->SetActorRelativeLocation(Position);
}

astro_body_t ASolarSystemManager::ToAstroBody(EPlanetBody Body)
{
	switch (Body)
	{
	case EPlanetBody::Mercury:	return BODY_MERCURY;
	case EPlanetBody::Venus:	return BODY_VENUS;
	case EPlanetBody::Earth:	return BODY_EARTH;
	case EPlanetBody::Moon:		return BODY_MOON;
	case EPlanetBody::Mars:		return BODY_MARS;
	case EPlanetBody::Jupiter:	return BODY_JUPITER;
	case EPlanetBody::Saturn:	return BODY_SATURN;
	case EPlanetBody::Uranus:	return BODY_URANUS;
	case EPlanetBody::Neptune:	return BODY_NEPTUNE;
	case EPlanetBody::Sun:		return BODY_SUN;
	default:					return BODY_INVALID;
	}
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
