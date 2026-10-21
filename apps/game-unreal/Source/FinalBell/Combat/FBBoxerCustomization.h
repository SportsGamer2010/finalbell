// Create-a-Boxer look profile. Version 1.
// These numbers are presentation. They do not change the capsule, the punch spheres, or CompetitiveFair ratings.
#pragma once

#include "CoreMinimal.h"
#include "Combat/FBFighterBalance.h"
#include "FBBoxerCustomization.generated.h"

/** Bone scales and face-mark offsets for one look. Length stays within 3 percent so the existing punch pose still reaches. */
USTRUCT(BlueprintType)
struct FFBVisualProportions
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float SpineLength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ShoulderLength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ShoulderBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ChestBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float WaistBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float UpperArmLength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float UpperArmBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ForearmBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ThighLength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ThighBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float CalfBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float NeckBulk = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float HeadWidth = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float HeadLength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float GloveBulk = 1.f;

	/** Centimeters on the head bone. Cosmetic marks only. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float BrowOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float JawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float NoseOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float CheekOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float ChinOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float EyeOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float LipOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float EarOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Asymmetry = 0.f;
};

/** Placeholder skin shading. A material shows these only when it has the matching parameters. */
USTRUCT(BlueprintType)
struct FFBDamageLook
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Sweat = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Bruise = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Swelling = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Cut = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Blood = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float BodyBruise = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Roughness = 0.48f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Creator")
	float Subsurface = 0.35f;
};

class FINALBELL_API FFBBoxerCustomization
{
public:
	static constexpr int32 ProfileVersion = 1;
	static constexpr int32 AgeMin = 18;
	static constexpr int32 AgeMax = 45;
	static constexpr int32 HometownMax = 48;
	/** Bone length stays inside this half-span so IK still meets the punch pose. */
	static constexpr float LengthHalfSpan = 0.03f;
	static constexpr float BulkHalfSpan = 0.10f;

	static void ResetLook(FFBPresentation& Look);
	static void Sanitize(FFBPresentation& Look);
	static bool Validate(const FFBPresentation& Look, EFBWeightClassId ClassId, FString& OutWhy);
	static void Randomize(FFBPresentation& Look, int32 Seed);
	static FFBVisualProportions ProportionsFor(const FFBPresentation& Look);
	static FFBDamageLook DamageFromFight(float HeadAlpha, float BodyAlpha, float Cut, float Bruise, float Sweat);

	static void HeightBandCm(EFBWeightClassId ClassId, float& OutMin, float& OutMax);
	static void ReachBandCm(EFBWeightClassId ClassId, float& OutMin, float& OutMax);
	static float DisplayHeightCm(const FFBPresentation& Look, EFBWeightClassId ClassId);
	static float DisplayReachCm(const FFBPresentation& Look, EFBWeightClassId ClassId);

	static const TCHAR* GloveStyleName(int32 Style);
	static const TCHAR* TrunksStyleName(int32 Style);
	static const TCHAR* ShoeStyleName(int32 Style);
	static const TCHAR* WrapStyleName(int32 Style);
	static const TCHAR* AccessoryName(int32 Style);

	static bool LengthsStayReachable(const FFBVisualProportions& Proportions);
	static bool BulksStayOnSkeleton(const FFBVisualProportions& Proportions);
};
