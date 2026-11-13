#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "PlanetArrowsComponent.generated.h"

class APlanetBase;
class UMaterialInstanceDynamic;

// Names shared by the arrows component, Shaders/NeonArrows.hlsl and the editor-only material builder.
namespace NeonArrows
{
	inline constexpr const TCHAR* MaterialFolder = TEXT("/Game/Materials");
	inline constexpr const TCHAR* MaterialName   = TEXT("M_NeonArrows");
	inline constexpr const TCHAR* MaterialPath   = TEXT("/Game/Materials/M_NeonArrows.M_NeonArrows");
	inline constexpr const TCHAR* ShaderFile     = TEXT("Shaders/NeonArrows.hlsl"); // relative to the project folder

	// Vector parameters (float3, also the Custom node input names).
	inline constexpr const TCHAR* OriginParam = TEXT("ArrowOrigin"); // system plane origin (world)
	inline constexpr const TCHAR* AxisXParam  = TEXT("ArrowAxisX");  // system plane X axis (world)
	inline constexpr const TCHAR* AxisYParam  = TEXT("ArrowAxisY");  // system plane Y axis (world)
	inline constexpr const TCHAR* SunParam    = TEXT("ArrowSun");    // Sun x, y (plane), shaft start offset
	inline constexpr const TCHAR* StyleParam  = TEXT("ArrowStyle");  // width px, glow px, head px
	inline constexpr const TCHAR* ColorParam  = TEXT("ArrowColor");  // rgb
	inline constexpr const TCHAR* SlotPrefix  = TEXT("Arrow");       // Arrow0..Arrow9: tip x, tip y (plane), visibility

	// Scalar parameters.
	inline constexpr const TCHAR* IntensityParam = TEXT("ArrowIntensity");
	inline constexpr const TCHAR* GlowStrengthParam = TEXT("ArrowGlowStrength");
	inline constexpr const TCHAR* HeadAngleParam = TEXT("ArrowHeadAngle"); // degrees

	// Must match the number of Arrow slots in Shaders/NeonArrows.hlsl.
	inline constexpr int32 MaxArrows = 10;

	// Translucency order: VisualizationPlane < arrows < everything else (default 0).
	inline constexpr int32 PlaneSortPriority = -2;
	inline constexpr int32 ArrowSortPriority = -1;
	inline constexpr const TCHAR* PlaneActorLabel = TEXT("VisualizationPlane");
}

// A flat quad over the system plane that draws a neon arrow from the Sun to every locked planet.
// The arrows are drawn by M_NeonArrows at a constant width in screen pixels, so zooming does not change them.
// ASolarSystemManager creates one at BeginPlay if its Blueprint doesn't already have one
// (add it to BP_SolarSystemManager to tune the values in Details).
UCLASS(ClassGroup = "Solar System", meta = (BlueprintSpawnableComponent))
class MRLAB_FINALPROJECT_API UPlanetArrowsComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPlanetArrowsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows")
	TSoftObjectPtr<UMaterialInterface> ArrowMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows")
	// Muted version of the M_HighlightOverlay red, so the highlighted planet still reads first.
	FLinearColor Color = FLinearColor(0.55f, 0.06f, 0.05f);

	// Emissive brightness. Bloom is off in this project, so the glow comes from the halo (GlowPx).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float Intensity = 1.2f;

	// Width of the solid core, in screen pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.5"))
	float WidthPx = 1.0f;

	// Falloff of the glow around the core, in screen pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float GlowPx = 2.5f;

	// Opacity of the glow right next to the core. 0 = plain line without a halo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlowStrength = 0.15f;

	// Length of the arrowhead arms, in screen pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float HeadPx = 10.0f;

	// Angle between the shaft and each arrowhead arm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float HeadAngleDeg = 28.0f;

	// Space left between the arrow and the Sun's / planet's surface, in world units.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float Gap = 15.0f;

	// How long an arrow takes to fade out once its planet is unlocked. 0 = instantly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float FadeSeconds = 0.5f;

	// Lift above the system plane, in world units.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrows", meta = (ClampMin = "0.0"))
	float HeightAbovePlane = 0.5f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateArrows(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArrowMID;

	// Current arrow opacity per planet, eased toward 0 (unlocked) or 1 (locked).
	TMap<TWeakObjectPtr<APlanetBase>, float> ArrowVisibility;

	TArray<FName> SlotParamNames;
	bool bWarnedTooManyPlanets = false;
};
