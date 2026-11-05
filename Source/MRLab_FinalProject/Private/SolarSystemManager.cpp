// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSystemManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

static double GetBodyRadiusKm(astro_body_t Body)
{
    switch (Body)
    {
        case BODY_SUN:     return SUN_RADIUS_KM;
        case BODY_MERCURY: return MERCURY_EQUATORIAL_RADIUS_KM;
        case BODY_VENUS:   return VENUS_RADIUS_KM;
        case BODY_EARTH:   return EARTH_EQUATORIAL_RADIUS_KM;
        case BODY_MOON:    return MOON_EQUATORIAL_RADIUS_KM;
        case BODY_MARS:    return MARS_EQUATORIAL_RADIUS_KM;
        case BODY_JUPITER: return JUPITER_EQUATORIAL_RADIUS_KM;
        case BODY_SATURN:  return SATURN_EQUATORIAL_RADIUS_KM;
        case BODY_URANUS:  return URANUS_EQUATORIAL_RADIUS_KM;
        case BODY_NEPTUNE: return NEPTUNE_EQUATORIAL_RADIUS_KM;
        default:           return 0.0;
    }
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

	CollectPlanets();

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
			AdoptPlanetDecorations(Planet);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SolarSystemManager: found %d planet(s)."), Planets.Num());
}

// Components authored under a planet's ChildActorComponent in the manager BP (rings, lights)
// stay at the origin, because only the child actor moves. Re-parent them onto the planet's mesh
// so they inherit its position and scale.
void ASolarSystemManager::AdoptPlanetDecorations(APlanetBase* Planet)
{
	USceneComponent* Anchor = Planet->GetParentComponent(); // the CAC that spawned this planet
	if (!Anchor || !Planet->Mesh) return;

	for (USceneComponent* Child : TArray<USceneComponent*>(Anchor->GetAttachChildren())) // copy: attaching mutates the array
	{
		if (Child->GetOwner() == this) // manager-owned only; skips the planet's own root
		{
			Child->AttachToComponent(Planet->Mesh, FAttachmentTransformRules::KeepWorldTransform);
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
	for (APlanetBase* Planet : Planets)
	{
		if (Planet) UpdateBody(Planet, AstroTime);
	}
}

void ASolarSystemManager::UpdateBody(APlanetBase* Planet, const astro_time_t& AstroTime)
{
	const astro_vector_t Vec = Astronomy_HelioVector(ToAstroBody(Planet->Body), AstroTime);
	if (Vec.status != ASTRO_SUCCESS) return;

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

	Planet->SetActorRelativeLocation(Position);

	const double BodyRadiusKm = GetBodyRadiusKm(ToAstroBody(Planet->Body));
	if (BodyRadiusKm > 0.0 && Planet->Mesh)
	{
		const double RadiusRatio = BodyRadiusKm / SUN_RADIUS_KM;
		const double VisualRatio = FMath::Pow(RadiusRatio, RadiusExponent);
		const FVector SunScale = (SunPlanet && SunPlanet->Mesh)
			? SunPlanet->Mesh->GetRelativeScale3D()
			: FVector(1.0);
		Planet->Mesh->SetRelativeScale3D(SunScale * VisualRatio);
	}
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

APlanetBase* ASolarSystemManager::FindPlanetInCone(FVector Origin, FVector Direction,
	float ConeHalfAngleDeg, float MaxDistance) const
{
	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero()) return nullptr;

	const double ConeHalfAngle = FMath::DegreesToRadians(ConeHalfAngleDeg);

	APlanetBase* Best = nullptr;
	double BestEdgeAngle = TNumericLimits<double>::Max();
	double BestDistance = TNumericLimits<double>::Max();

	for (APlanetBase* Planet : Planets)
	{
		if (!Planet || !Planet->Mesh) continue;

		const FVector ToPlanet = Planet->GetActorLocation() - Origin;
		const double Distance = ToPlanet.Size();
		if (Distance < KINDA_SMALL_NUMBER || Distance > MaxDistance) continue;

		const double AngleToCenter = FMath::Acos(FMath::Clamp(Dir | (ToPlanet / Distance), -1.0, 1.0));

		// How large the planet appears from here (half of its visible size).
		const double Radius = Planet->Mesh->Bounds.BoxExtent.GetMax();
		const double AngularRadius = (Radius >= Distance) ? PI : FMath::Asin(Radius / Distance);

		// 0 when looking at the planet's disk, otherwise the angle to its nearest edge.
		const double AngleToEdge = FMath::Max(0.0, AngleToCenter - AngularRadius);
		if (AngleToEdge > ConeHalfAngle) continue;

		// Prefer the planet closest to the view direction; on a tie, the nearer one.
		const bool bBetterAngle = AngleToEdge < BestEdgeAngle - KINDA_SMALL_NUMBER;
		const bool bSameAngleButCloser =
			FMath::IsNearlyEqual(AngleToEdge, BestEdgeAngle) && Distance < BestDistance;

		if (bBetterAngle || bSameAngleButCloser)
		{
			Best = Planet;
			BestEdgeAngle = AngleToEdge;
			BestDistance = Distance;
		}
	}

	return Best;
}