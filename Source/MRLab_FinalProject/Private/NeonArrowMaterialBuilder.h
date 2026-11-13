#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"

class UMaterial;

// Editor-only: generates the arrow material, so it lives in code (Shaders/NeonArrows.hlsl) instead of a hand-built graph.
struct FNeonArrowMaterialBuilder
{
	// Creates /Game/Materials/M_NeonArrows, or rebuilds its graph in place, and saves it.
	// Returns nullptr (and logs why) on failure.
	static UMaterial* BuildOrUpdate();
};

#endif // WITH_EDITOR
