#include "NeonArrowMaterialBuilder.h"

#if WITH_EDITOR

#include "PlanetArrowsComponent.h"

#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "FileHelpers.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// Adds an input named InputName to the Custom node and wires Expression's first output into it.
	void AddCustomInput(UMaterialExpressionCustom* Custom, const TCHAR* InputName, UMaterialExpression* Expression)
	{
		FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
		Input.InputName = FName(InputName);
		Input.Input.Connect(0, Expression);
	}

	UMaterialExpressionVectorParameter* AddVectorParam(UMaterial* Material, const TCHAR* Name,
		const FLinearColor& Default, int32 Y)
	{
		auto* Param = Cast<UMaterialExpressionVectorParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionVectorParameter::StaticClass(), -800, Y));
		Param->ParameterName = FName(Name);
		Param->DefaultValue = Default;
		return Param;
	}

	UMaterialExpressionScalarParameter* AddScalarParam(UMaterial* Material, const TCHAR* Name, float Default, int32 Y)
	{
		auto* Param = Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionScalarParameter::StaticClass(), -800, Y));
		Param->ParameterName = FName(Name);
		Param->DefaultValue = Default;
		return Param;
	}

	UMaterialExpressionComponentMask* AddMask(UMaterial* Material, UMaterialExpression* Source,
		bool R, bool G, bool B, bool A, int32 Y)
	{
		auto* Mask = Cast<UMaterialExpressionComponentMask>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionComponentMask::StaticClass(), -150, Y));
		Mask->R = R;
		Mask->G = G;
		Mask->B = B;
		Mask->A = A;
		Mask->Input.Connect(0, Source);
		return Mask;
	}
}

UMaterial* FNeonArrowMaterialBuilder::BuildOrUpdate()
{
	const FString ShaderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / NeonArrows::ShaderFile);
	FString Code;
	if (!FFileHelper::LoadFileToString(Code, *ShaderPath))
	{
		UE_LOG(LogTemp, Error, TEXT("NeonArrows: can't read %s"), *ShaderPath);
		return nullptr;
	}

	UMaterial* Material = LoadObject<UMaterial>(nullptr, NeonArrows::MaterialPath, nullptr, LOAD_NoWarn);
	if (!Material)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		Material = Cast<UMaterial>(AssetTools.CreateAsset(NeonArrows::MaterialName, NeonArrows::MaterialFolder,
			UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("NeonArrows: can't create %s"), NeonArrows::MaterialPath);
			return nullptr;
		}
	}

	Material->Modify();
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	// Unlit translucent overlay; depth test stays on so opaque planets hide the arrows.
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	Material->bUseTranslucencyVertexFog = false;

	auto* WorldPos = Cast<UMaterialExpressionWorldPosition>(UMaterialEditingLibrary::CreateMaterialExpression(
		Material, UMaterialExpressionWorldPosition::StaticClass(), -800, -200));
	WorldPos->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;

	auto* Custom = Cast<UMaterialExpressionCustom>(UMaterialEditingLibrary::CreateMaterialExpression(
		Material, UMaterialExpressionCustom::StaticClass(), -400, 0));
	Custom->Description = TEXT("NeonArrows (Shaders/NeonArrows.hlsl)");
	Custom->Code = Code;
	Custom->OutputType = CMOT_Float4;
	Custom->Inputs.Reset();

	// Defaults are only placeholders: UPlanetArrowsComponent sets every parameter each frame.
	int32 Y = -100;
	AddCustomInput(Custom, TEXT("P"), WorldPos);
	AddCustomInput(Custom, NeonArrows::OriginParam, AddVectorParam(Material, NeonArrows::OriginParam, FLinearColor(0, 0, 0, 0), Y += 100));
	AddCustomInput(Custom, NeonArrows::AxisXParam, AddVectorParam(Material, NeonArrows::AxisXParam, FLinearColor(1, 0, 0, 0), Y += 100));
	AddCustomInput(Custom, NeonArrows::AxisYParam, AddVectorParam(Material, NeonArrows::AxisYParam, FLinearColor(0, 1, 0, 0), Y += 100));
	AddCustomInput(Custom, NeonArrows::SunParam, AddVectorParam(Material, NeonArrows::SunParam, FLinearColor(0, 0, 0, 0), Y += 100));
	AddCustomInput(Custom, NeonArrows::StyleParam, AddVectorParam(Material, NeonArrows::StyleParam, FLinearColor(1.0f, 2.5f, 6, 0), Y += 100));
	AddCustomInput(Custom, NeonArrows::ColorParam, AddVectorParam(Material, NeonArrows::ColorParam, FLinearColor(0.81f, 0.37f, 0.07f, 1.0f), Y += 100));
	AddCustomInput(Custom, NeonArrows::IntensityParam, AddScalarParam(Material, NeonArrows::IntensityParam, 1.2f, Y += 100));
	AddCustomInput(Custom, NeonArrows::GlowStrengthParam, AddScalarParam(Material, NeonArrows::GlowStrengthParam, 0.15f, Y += 100));
	AddCustomInput(Custom, NeonArrows::HeadAngleParam, AddScalarParam(Material, NeonArrows::HeadAngleParam, 25.0f, Y += 100));
	for (int32 Slot = 0; Slot < NeonArrows::MaxArrows; ++Slot)
	{
		const FString Name = FString::Printf(TEXT("%s%d"), NeonArrows::SlotPrefix, Slot);
		AddCustomInput(Custom, *Name, AddVectorParam(Material, *Name, FLinearColor(0, 0, 0, 0), Y += 100));
	}

	UMaterialExpressionComponentMask* EmissiveMask = AddMask(Material, Custom, true, true, true, false, -50);
	UMaterialExpressionComponentMask* OpacityMask = AddMask(Material, Custom, false, false, false, true, 100);
	UMaterialEditingLibrary::ConnectMaterialProperty(EmissiveMask, TEXT(""), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(OpacityMask, TEXT(""), MP_Opacity);

	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->MarkPackageDirty();

	if (!UEditorLoadingAndSavingUtils::SavePackages({ Material->GetPackage() }, false))
	{
		UE_LOG(LogTemp, Warning, TEXT("NeonArrows: %s built but not saved, save it manually."), NeonArrows::MaterialPath);
	}

	UE_LOG(LogTemp, Log, TEXT("NeonArrows: built %s from %s"), NeonArrows::MaterialPath, *ShaderPath);
	return Material;
}

#endif // WITH_EDITOR
