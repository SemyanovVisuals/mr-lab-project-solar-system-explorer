// Fill out your copyright notice in the Description page of Project Settings.

#include "SolarSystemManager.h"
#include "PlanetArrowsComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "NeonArrowMaterialBuilder.h"
#include "EngineUtils.h"
#endif

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

// Astronomy Engine reports EQJ (J2000 equatorial) vectors; the scene uses ecliptic axes with Y flipped.
static FVector EqjToScene(const astro_rotation_t& EqjToEcl, const FVector& V)
{
	const astro_vector_t E = Astronomy_RotateVector(EqjToEcl, { ASTRO_SUCCESS, V.X, V.Y, V.Z });
	return FVector(E.x, -E.y, E.z);
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

	// Neon arrows to the locked planets. Created here unless the Blueprint already has one (to tune it in Details).
	if (!FindComponentByClass<UPlanetArrowsComponent>())
	{
		UPlanetArrowsComponent* Arrows = NewObject<UPlanetArrowsComponent>(this, TEXT("PlanetArrows"));
		Arrows->SetupAttachment(GetRootComponent());
		Arrows->RegisterComponent();
	}
}

#if WITH_EDITOR
void ASolarSystemManager::BuildArrowAssets()
{
	if (!FNeonArrowMaterialBuilder::BuildOrUpdate())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bool bFoundPlane = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetActorLabel() != NeonArrows::PlaneActorLabel)
		{
			continue;
		}

		TArray<UPrimitiveComponent*> Primitives;
		It->GetComponents(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive->TranslucencySortPriority != NeonArrows::PlaneSortPriority)
			{
				Primitive->Modify();
				Primitive->SetTranslucentSortPriority(NeonArrows::PlaneSortPriority);
			}
		}
		bFoundPlane = true;
	}

	if (bFoundPlane)
	{
		UE_LOG(LogTemp, Log, TEXT("NeonArrows: %s sort priority set to %d. Save the level to keep it."),
			NeonArrows::PlaneActorLabel, NeonArrows::PlaneSortPriority);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NeonArrows: no actor labelled '%s' in this level. Set its Translucency Sort Priority to %d by hand."),
			NeonArrows::PlaneActorLabel, NeonArrows::PlaneSortPriority);
	}
}
#endif

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

void ASolarSystemManager::SetSimulatedOffsetYears(double OffsetYears)
{
	OffsetYears = FMath::Clamp(OffsetYears, -MaxOffsetYears, MaxOffsetYears);
	if (OffsetYears == CurrentOffsetYears) return; // e.g. holding scroll at the limit
	CurrentOffsetYears = OffsetYears;

	const double OffsetDays = OffsetYears * 365.25;
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
	const astro_body_t Body = ToAstroBody(Planet->Body);
	const astro_rotation_t EqjToEcl = Astronomy_Rotation_EQJ_ECL();

	// Ecliptic coordinates, so dropping Z below flattens the orbits onto the ecliptic plane.
	const astro_vector_t Vec = Astronomy_RotateVector(EqjToEcl, Astronomy_HelioVector(Body, AstroTime));
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

	// Placed in the manager's frame, so a transform authored on the planet's ChildActorComponent can't move it off the plane.
	Planet->SetActorLocation(GetActorTransform().TransformPosition(Position));

	// Orient from the IAU rotation model: Z = north pole, Y = prime meridian (the planet meshes have
	// longitude 0 on local +Y), i.e. the body's equator/EQJ equator node rotated about the pole by the spin angle W.
	astro_time_t Time = AstroTime;
	const astro_axis_t Axis = Astronomy_RotationAxis(Body, &Time);
	if (Axis.status == ASTRO_SUCCESS)
	{
		const double RaRad = FMath::DegreesToRadians(Axis.ra * 15.0); // ra is in hours
		const double W = FMath::DegreesToRadians(Axis.spin);
		const FVector Pole(Axis.north.x, Axis.north.y, Axis.north.z);
		const FVector Node(-FMath::Sin(RaRad), FMath::Cos(RaRad), 0.0);
		const FVector Meridian = Node * FMath::Cos(W) + (Pole ^ Node) * FMath::Sin(W);

		Planet->SetActorRotation(GetActorQuat() *
			FRotationMatrix::MakeFromYZ(EqjToScene(EqjToEcl, Meridian), EqjToScene(EqjToEcl, Pole)).ToQuat());
	}

	const double BodyRadiusKm = GetBodyRadiusKm(Body);
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