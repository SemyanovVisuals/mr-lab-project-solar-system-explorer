// PlanetBase.cpp
#include "PlanetBase.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

APlanetBase::APlanetBase()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

void APlanetBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UE_LOG(LogTemp, Warning, TEXT("PlanetBase OnConstruction: %s, Material = %s"),
		*GetName(), PlanetMaterial ? *PlanetMaterial->GetName() : TEXT("NONE"));

	if (PlanetMaterial)
	{
		Mesh->SetMaterial(0, PlanetMaterial);
	}
}

// Called when the game starts or when spawned
void APlanetBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlanetBase::SetHighlighted(bool bHighlighted)
{
	if (Mesh)
	{
		Mesh->SetOverlayMaterial(bHighlighted ? HighlightOverlayMaterial : nullptr);
	}
}

float APlanetBase::GetRadius() const
{
	return Mesh ? static_cast<float>(Mesh->Bounds.BoxExtent.GetMax()) : 0.0f;
}

float APlanetBase::GetViewDistanceForFill(float Fill, float ScreenHalfHeight, float ViewerToScreenDistance) const
{
	const float Radius = GetRadius();
	if (Radius <= 0.0f || ScreenHalfHeight <= 0.0f || ViewerToScreenDistance <= 0.0f)
	{
		return 0.0f;
	}

	// Angle (from the screen's center) at which the planet's edge should appear.
	const float ClampedFill = FMath::Clamp(Fill, 0.01f, 1.0f);
	const float HalfAngle = FMath::Atan(ClampedFill * ScreenHalfHeight / ViewerToScreenDistance);

	return Radius / FMath::Sin(HalfAngle);
}

bool APlanetBase::isSun()
{
	return bIsSun;
}