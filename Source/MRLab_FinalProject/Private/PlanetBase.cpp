// PlanetBase.cpp
#include "PlanetBase.h"
#include "Components/StaticMeshComponent.h"

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


