// Custom node body of /Game/Materials/M_NeonArrows.
// The material is generated from this file: after editing it, select the Solar System Manager
// in the level and press "Build Arrow Assets" (see FNeonArrowMaterialBuilder).
//
// Draws a neon arrow from the Sun to each enabled slot, at a constant width in screen pixels.
// Inputs (names must match NeonArrows:: in PlanetArrowsComponent.h):
//   P                                    absolute world position of the pixel
//   ArrowOrigin, ArrowAxisX, ArrowAxisY  system plane frame in world space
//   ArrowSun                             Sun x, y on the plane, distance from the Sun's center to the shaft start
//   ArrowStyle                           core width px, glow falloff px, arrowhead arm length px
//   ArrowColor, ArrowIntensity           neon color and emissive brightness
//   ArrowGlowStrength                    opacity of the halo next to the core (0..1)
//   ArrowHeadAngle                       angle between the shaft and each arrowhead arm, degrees
//   Arrow0 .. Arrow9                     tip x, tip y on the plane, visibility (0 = no arrow)
// Output: float4(emissive rgb, opacity).

struct FNeonArrowFunctions
{
	// Keeps the segment a-b if it is closer to p than the current best.
	void Segment(float2 p, float2 a, float2 b, inout float Best, inout float2 Closest)
	{
		float2 ba = b - a;
		float h = saturate(dot(p - a, ba) / max(dot(ba, ba), 1e-6));
		float2 c = a + ba * h;
		float d = length(p - c);
		if (d < Best)
		{
			Best = d;
			Closest = c;
		}
	}
};
FNeonArrowFunctions F;

// Pixel position in system plane coordinates (world units).
float3 Rel = P - ArrowOrigin;
float2 p = float2(dot(Rel, ArrowAxisX), dot(Rel, ArrowAxisY));

// World units covered by one screen pixel here: sizes the arrowhead in pixels.
float WorldPerPixel = max(length(ddx(p)), length(ddy(p)));
float HeadLength = ArrowStyle.z * WorldPerPixel;
float HeadCos = cos(radians(ArrowHeadAngle));
float HeadSin = sin(radians(ArrowHeadAngle));

float3 Arrows[10] = { Arrow0, Arrow1, Arrow2, Arrow3, Arrow4, Arrow5, Arrow6, Arrow7, Arrow8, Arrow9 };

float Best = 1e20;
float2 Closest = p;
float Visibility = 0.0;
for (int i = 0; i < 10; i++)
{
	float3 A = Arrows[i];
	if (A.z <= 0.0)
	{
		continue; // same for every pixel: no divergence
	}

	float2 Tip = A.xy;
	float2 Dir = normalize(Tip - ArrowSun.xy);
	float2 Side = float2(-Dir.y, Dir.x);
	float2 Start = ArrowSun.xy + Dir * ArrowSun.z;
	float2 Back = -Dir * HeadCos;

	float PrevBest = Best;
	F.Segment(p, Start, Tip, Best, Closest);
	F.Segment(p, Tip, Tip + (Back + Side * HeadSin) * HeadLength, Best, Closest);
	F.Segment(p, Tip, Tip + (Back - Side * HeadSin) * HeadLength, Best, Closest);

	if (Best < PrevBest)
	{
		Visibility = A.z;
	}
}

// World distance -> screen pixels: how many world units one pixel covers in the direction away from the line.
// Uses only the derivatives of p (smooth everywhere), so creases where the shaft meets the head
// don't produce artifacts. Keeps the width constant at any zoom and viewing angle.
float2 N = (Best > 1e-6) ? (p - Closest) / Best : float2(1.0, 0.0);
float WorldPerPixelN = length(float2(dot(ddx(p), N), dot(ddy(p), N)));
float PixelDistance = Best / max(WorldPerPixelN, 1e-8);

float HalfWidth = ArrowStyle.x * 0.5;
float Core = 1.0 - smoothstep(HalfWidth - 0.75, HalfWidth + 0.75, PixelDistance);
float Glow = exp2(-PixelDistance / max(ArrowStyle.y, 0.01)) * ArrowGlowStrength;

float Alpha = saturate(Core * 0.85 + Glow) * Visibility;
if (Alpha < 0.002)
{
	return float4(0.0, 0.0, 0.0, 0.0);
}

// Slightly whiter core, colored halo.
float3 HotCore = lerp(ArrowColor, float3(1.0, 1.0, 1.0), 0.15);
float3 Emissive = (ArrowColor * Glow + HotCore * Core) * ArrowIntensity;
return float4(Emissive, Alpha);
