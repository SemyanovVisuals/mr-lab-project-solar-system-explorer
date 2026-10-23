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

	UFUNCTION(BlueprintCallable, Category = "Planet")
	void SetHighlighted(bool bHighlighted);

	UPROPERTY(EditAnywhere, Category = "Planet|Highlight")
	UMaterialInterface* HighlightOverlayMaterial = nullptr;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Planet")
	float GetRadius() const;

	// Distance from the planet's center at which it fills the given fraction of the screen height.
	UFUNCTION(BlueprintPure, Category = "Planet")
	float GetViewDistanceForFill(float Fill, float ScreenHalfHeight, float ViewerToScreenDistance) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
