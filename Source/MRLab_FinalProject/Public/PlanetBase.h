#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetBase.generated.h"

UENUM(BlueprintType)
enum class EPlanetBody : uint8
{
	Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus, Neptune, Sun
};

UCLASS()
class MRLAB_FINALPROJECT_API APlanetBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlanetBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (ExposeOnSpawn = true))
	EPlanetBody Body = EPlanetBody::Earth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (ExposeOnSpawn = true))
	UMaterialInterface* PlanetMaterial = nullptr;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
