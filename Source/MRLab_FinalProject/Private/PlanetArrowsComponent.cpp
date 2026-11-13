#include "PlanetArrowsComponent.h"

#include "PlanetBase.h"
#include "SolarSystemManager.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UPlanetArrowsComponent::UPlanetArrowsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork; // after the planets have moved this frame

	// 100 x 100 units in XY, facing +Z.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		SetStaticMesh(PlaneMesh.Object);
	}

	ArrowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(NeonArrows::MaterialPath));

	// Placed in world space every tick from the manager's frame, independent of the parent's scale.
	Mobility = EComponentMobility::Movable;
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);

	// Purely visual: no collision, shadows, decals or captures.
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CanCharacterStepUpOn = ECB_No;
	CastShadow = false;
	bReceivesDecals = false;
	bUseAsOccluder = false;
	bVisibleInReflectionCaptures = false;
	bVisibleInRealTimeSkyCaptures = false;
	bVisibleInRayTracing = false;

	// Above the VisualizationPlane, below every other translucent object (quiz UI, corona, ...).
	TranslucencySortPriority = NeonArrows::ArrowSortPriority;
}

void UPlanetArrowsComponent::BeginPlay()
{
	Super::BeginPlay();

	SlotParamNames.Reset(NeonArrows::MaxArrows);
	for (int32 Slot = 0; Slot < NeonArrows::MaxArrows; ++Slot)
	{
		SlotParamNames.Add(FName(*FString::Printf(TEXT("%s%d"), NeonArrows::SlotPrefix, Slot)));
	}

	UMaterialInterface* Material = ArrowMaterial.LoadSynchronous();
	if (!Material)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PlanetArrows: %s not found. Select the Solar System Manager in the level and press 'Build Arrow Assets'."),
			NeonArrows::MaterialPath);
		SetVisibility(false);
		SetComponentTickEnabled(false);
		return;
	}

	ArrowMID = CreateDynamicMaterialInstance(0, Material);
	UpdateArrows(0.0f);
}

void UPlanetArrowsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateArrows(DeltaTime);
}

void UPlanetArrowsComponent::UpdateArrows(float DeltaTime)
{
	const ASolarSystemManager* Manager = Cast<ASolarSystemManager>(GetOwner());
	if (!Manager || !ArrowMID)
	{
		return;
	}

	// The planets lie on the manager's local XY plane (see ASolarSystemManager::UpdateBody).
	const FVector Origin = Manager->GetActorLocation();
	const FQuat Rotation = Manager->GetActorQuat();
	const FVector AxisX = Rotation.GetAxisX();
	const FVector AxisY = Rotation.GetAxisY();

	auto ToPlane = [&](const FVector& WorldPosition)
	{
		const FVector Rel = WorldPosition - Origin;
		return FVector2D(Rel | AxisX, Rel | AxisY);
	};

	const TArray<APlanetBase*>& Planets = Manager->GetPlanets();

	const APlanetBase* Sun = nullptr;
	for (const APlanetBase* Planet : Planets)
	{
		if (IsValid(Planet) && Planet->bIsSun)
		{
			Sun = Planet;
			break;
		}
	}

	const FVector2D SunPos = Sun ? ToPlane(Sun->GetActorLocation()) : FVector2D::ZeroVector;
	const double StartOffset = (Sun ? Sun->GetRadius() : 0.0) + Gap;

	const float FadeStep = (FadeSeconds > 0.0f) ? DeltaTime / FadeSeconds : 1.0f;

	int32 Slot = 0;
	bool bAnyVisible = false;
	double Extent = 0.0; // half-size the quad needs to cover every planet

	for (APlanetBase* Planet : Planets)
	{
		if (!IsValid(Planet) || Planet == Sun)
		{
			continue;
		}

		const FVector2D Center = ToPlane(Planet->GetActorLocation());
		const double Radius = Planet->GetRadius();
		Extent = FMath::Max(Extent, Center.GetAbsMax() + Radius);

		// Ease toward 1 while locked, toward 0 once unlocked. New planets start at their target.
		const float Target = Planet->bIsUnlocked ? 0.0f : 1.0f;
		float& Vis = ArrowVisibility.FindOrAdd(Planet, Target);
		Vis = (Target > Vis) ? FMath::Min(Vis + FadeStep, Target) : FMath::Max(Vis - FadeStep, Target);

		if (Slot >= NeonArrows::MaxArrows)
		{
			if (!bWarnedTooManyPlanets)
			{
				UE_LOG(LogTemp, Warning, TEXT("PlanetArrows: more than %d planets, the rest get no arrow."),
					NeonArrows::MaxArrows);
				bWarnedTooManyPlanets = true;
			}
			continue;
		}

		// Tip just outside the planet, on the line from the Sun's center.
		FLinearColor SlotValue(0.0f, 0.0f, 0.0f, 0.0f);
		const FVector2D ToPlanet = Center - SunPos;
		const double Distance = ToPlanet.Size();
		const double TipDistance = Distance - Radius - Gap;
		if (Vis > 0.0f && TipDistance > StartOffset)
		{
			const FVector2D Tip = SunPos + ToPlanet / Distance * TipDistance;
			SlotValue = FLinearColor(Tip.X, Tip.Y, Vis, 0.0f);
			bAnyVisible = true;
		}
		ArrowMID->SetVectorParameterValue(SlotParamNames[Slot], SlotValue);
		++Slot;
	}

	for (; Slot < NeonArrows::MaxArrows; ++Slot)
	{
		ArrowMID->SetVectorParameterValue(SlotParamNames[Slot], FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	}

	// Nothing to draw: skip the full-system translucent quad entirely.
	if (GetVisibleFlag() != bAnyVisible)
	{
		SetVisibility(bAnyVisible);
	}
	if (!bAnyVisible)
	{
		return;
	}

	// Square quad centered on the manager, slightly above the plane. The engine plane is 100 units wide.
	const double HalfSize = FMath::Max(Extent * 1.1, 100.0);
	const FTransform QuadTransform(
		Rotation,
		Origin + Rotation.GetAxisZ() * HeightAbovePlane,
		FVector(HalfSize / 50.0, HalfSize / 50.0, 1.0));
	if (!GetComponentTransform().Equals(QuadTransform, 0.01))
	{
		SetWorldTransform(QuadTransform);
	}

	ArrowMID->SetVectorParameterValue(NeonArrows::OriginParam, FLinearColor(Origin.X, Origin.Y, Origin.Z, 0.0f));
	ArrowMID->SetVectorParameterValue(NeonArrows::AxisXParam, FLinearColor(AxisX.X, AxisX.Y, AxisX.Z, 0.0f));
	ArrowMID->SetVectorParameterValue(NeonArrows::AxisYParam, FLinearColor(AxisY.X, AxisY.Y, AxisY.Z, 0.0f));
	ArrowMID->SetVectorParameterValue(NeonArrows::SunParam, FLinearColor(SunPos.X, SunPos.Y, StartOffset, 0.0f));
	ArrowMID->SetVectorParameterValue(NeonArrows::StyleParam, FLinearColor(WidthPx, GlowPx, HeadPx, 0.0f));
	ArrowMID->SetVectorParameterValue(NeonArrows::ColorParam, Color);
	ArrowMID->SetScalarParameterValue(NeonArrows::IntensityParam, Intensity);
	ArrowMID->SetScalarParameterValue(NeonArrows::GlowStrengthParam, GlowStrength);
	ArrowMID->SetScalarParameterValue(NeonArrows::HeadAngleParam, HeadAngleDeg);
}
