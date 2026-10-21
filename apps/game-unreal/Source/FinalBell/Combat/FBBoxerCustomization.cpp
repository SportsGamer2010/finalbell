#include "Combat/FBBoxerCustomization.h"

namespace
{
	float Clamp01(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	float Around(float Slider, float HalfSpan)
	{
		return 1.f + (Clamp01(Slider) - 0.5f) * 2.f * HalfSpan;
	}

	float SignedCm(float Slider, float Span)
	{
		return (Clamp01(Slider) - 0.5f) * 2.f * Span;
	}

	void ClampInt(int32& Value, int32 Min, int32 Max)
	{
		Value = FMath::Clamp(Value, Min, Max);
	}
}

void FFBBoxerCustomization::ResetLook(FFBPresentation& Look)
{
	Look = FFBPresentation();
	Look.LookVersion = ProfileVersion;
	Look.Age = 28;
	Look.Country = TEXT("USA");
	Look.Hometown.Reset();
}

void FFBBoxerCustomization::Sanitize(FFBPresentation& Look)
{
	if (Look.LookVersion < 1)
	{
		Look.LookVersion = ProfileVersion;
	}
	if (Look.LookVersion > ProfileVersion)
	{
		return;
	}

	Look.Age = FMath::Clamp(Look.Age, AgeMin, AgeMax);
	Look.Hometown = Look.Hometown.Left(HometownMax);
	Look.Nickname = Look.Nickname.Left(24);
	Look.Country.TrimStartAndEndInline();
	if (Look.Country.IsEmpty())
	{
		Look.Country = TEXT("USA");
	}

	Look.Brow = Clamp01(Look.Brow);
	Look.Jaw = Clamp01(Look.Jaw);
	Look.Nose = Clamp01(Look.Nose);
	Look.Tattoos = Clamp01(Look.Tattoos);
	Look.Muscle = Clamp01(Look.Muscle);
	Look.Height = Clamp01(Look.Height);
	Look.Muscularity = Clamp01(Look.Muscularity);
	Look.BodyFat = Clamp01(Look.BodyFat);
	Look.Masculine = Clamp01(Look.Masculine);
	Look.Chest = Clamp01(Look.Chest);
	Look.Waist = Clamp01(Look.Waist);
	Look.Hip = Clamp01(Look.Hip);
	Look.Bicep = Clamp01(Look.Bicep);
	Look.Thigh = Clamp01(Look.Thigh);
	Look.Eyes = Clamp01(Look.Eyes);
	Look.Cheeks = Clamp01(Look.Cheeks);
	Look.Chin = Clamp01(Look.Chin);
	Look.Lips = Clamp01(Look.Lips);
	Look.Ears = Clamp01(Look.Ears);
	Look.Asymmetry = Clamp01(Look.Asymmetry);
	Look.SkinU = Clamp01(Look.SkinU);
	Look.SkinV = Clamp01(Look.SkinV);
	Look.FreckleDensity = Clamp01(Look.FreckleDensity);
	Look.IrisHue = Clamp01(Look.IrisHue);
	Look.HairGray = Clamp01(Look.HairGray);
	Look.Foundation = Clamp01(Look.Foundation);
	Look.Blush = Clamp01(Look.Blush);
	Look.LipMakeup = Clamp01(Look.LipMakeup);
	Look.HeadWidth = Clamp01(Look.HeadWidth);
	Look.HeadLength = Clamp01(Look.HeadLength);
	Look.ShoulderWidth = Clamp01(Look.ShoulderWidth);
	Look.TorsoLength = Clamp01(Look.TorsoLength);
	Look.ArmLength = Clamp01(Look.ArmLength);
	Look.LegLength = Clamp01(Look.LegLength);
	Look.Neck = Clamp01(Look.Neck);
	Look.Reach = Clamp01(Look.Reach);
	Look.ToothLength = FMath::Clamp(Look.ToothLength, -1.f, 1.f);
	Look.Muscle = Look.Muscularity;

	ClampInt(Look.SexPresentation, 0, 2);
	ClampInt(Look.GloveColor, 0, 5);
	ClampInt(Look.TrunksColor, 0, 5);
	ClampInt(Look.TrunksStripe, 0, 5);
	ClampInt(Look.ShoeColor, 0, 5);
	ClampInt(Look.WrapColor, 0, 5);
	ClampInt(Look.TattooPlace, 0, 3);
	ClampInt(Look.Scar, 0, 3);
	ClampInt(Look.FreckleMask, 0, 3);
	ClampInt(Look.IrisPattern, 0, 8);
	ClampInt(Look.Beard, 0, 3);
	ClampInt(Look.Mustache, 0, 1);
	ClampInt(Look.EyelashType, 0, 6);
	ClampInt(Look.GloveStyle, 0, 2);
	ClampInt(Look.TrunksStyle, 0, 2);
	ClampInt(Look.ShoeStyle, 0, 1);
	ClampInt(Look.WrapStyle, 0, 2);
	ClampInt(Look.Accessory, 0, 3);
	if (Look.GroomSlot.IsEmpty())
	{
		Look.GroomSlot = TEXT("Hair");
	}
}

bool FFBBoxerCustomization::Validate(const FFBPresentation& Look, EFBWeightClassId ClassId, FString& OutWhy)
{
	if (Look.LookVersion > ProfileVersion)
	{
		OutWhy = TEXT("That look was saved by a newer version of Final Bell.");
		return false;
	}
	if (Look.LookVersion < 1)
	{
		OutWhy = TEXT("That look has no version.");
		return false;
	}
	if (Look.Age < AgeMin || Look.Age > AgeMax)
	{
		OutWhy = FString::Printf(TEXT("Age stays between %d and %d."), AgeMin, AgeMax);
		return false;
	}
	if (Look.Hometown.Len() > HometownMax)
	{
		OutWhy = TEXT("Hometown is too long.");
		return false;
	}
	if (Look.Country.IsEmpty())
	{
		OutWhy = TEXT("Pick a country.");
		return false;
	}

	const float Height = DisplayHeightCm(Look, ClassId);
	const float Reach = DisplayReachCm(Look, ClassId);
	float HeightMin = 0.f;
	float HeightMax = 0.f;
	float ReachMin = 0.f;
	float ReachMax = 0.f;
	HeightBandCm(ClassId, HeightMin, HeightMax);
	ReachBandCm(ClassId, ReachMin, ReachMax);
	if (Height < HeightMin - 0.1f || Height > HeightMax + 0.1f || Reach < ReachMin - 0.1f || Reach > ReachMax + 0.1f)
	{
		OutWhy = TEXT("Height and reach have to stay inside the weight class.");
		return false;
	}

	const FFBVisualProportions Proportions = ProportionsFor(Look);
	if (!LengthsStayReachable(Proportions) || !BulksStayOnSkeleton(Proportions))
	{
		OutWhy = TEXT("That build would pull the gloves off the punch pose.");
		return false;
	}
	return true;
}

void FFBBoxerCustomization::Randomize(FFBPresentation& Look, int32 Seed)
{
	FRandomStream Stream(Seed);
	const FString Nickname = Look.Nickname;
	const FString Country = Look.Country;
	const int32 Sex = Look.SexPresentation;
	ResetLook(Look);
	Look.Nickname = Nickname;
	if (!Country.IsEmpty())
	{
		Look.Country = Country;
	}
	Look.SexPresentation = Sex;
	Look.Masculine = Sex / 2.f;

	const TCHAR* Towns[] = {
		TEXT("Detroit"), TEXT("Philadelphia"), TEXT("Mexico City"), TEXT("Havana"),
		TEXT("London"), TEXT("Kyiv"), TEXT("Lagos"), TEXT("Osaka")
	};
	Look.Hometown = Towns[Stream.RandRange(0, 7)];
	Look.Age = Stream.RandRange(AgeMin, AgeMax);
	Look.SkinU = Stream.FRand();
	Look.SkinV = Stream.FRand();
	Look.Height = Stream.FRand();
	Look.Reach = Stream.FRand();
	Look.Muscularity = Stream.FRand();
	Look.Muscle = Look.Muscularity;
	Look.BodyFat = Stream.FRand();
	Look.Chest = Stream.FRand();
	Look.Waist = Stream.FRand();
	Look.Hip = Stream.FRand();
	Look.Bicep = Stream.FRand();
	Look.Thigh = Stream.FRand();
	Look.ShoulderWidth = Stream.FRand();
	Look.TorsoLength = Stream.FRand();
	Look.ArmLength = Stream.FRand();
	Look.LegLength = Stream.FRand();
	Look.Neck = Stream.FRand();
	Look.HeadWidth = Stream.FRand();
	Look.HeadLength = Stream.FRand();
	Look.Brow = Stream.FRand();
	Look.Eyes = Stream.FRand();
	Look.Nose = Stream.FRand();
	Look.Cheeks = Stream.FRand();
	Look.Jaw = Stream.FRand();
	Look.Chin = Stream.FRand();
	Look.Lips = Stream.FRand();
	Look.Ears = Stream.FRand();
	Look.Asymmetry = Stream.FRand();
	Look.Hair = static_cast<EFBHairStyle>(Stream.RandRange(0, 4));
	Look.Beard = Stream.RandRange(0, 3);
	Look.Mustache = Stream.RandRange(0, 1);
	Look.HairGray = Stream.FRandRange(0.f, 0.45f);
	Look.IrisPattern = Stream.RandRange(0, 8);
	Look.FreckleMask = Stream.RandRange(0, 3);
	Look.FreckleDensity = Stream.FRandRange(0.f, 0.6f);
	Look.GloveColor = Stream.RandRange(0, 5);
	Look.TrunksColor = Stream.RandRange(0, 5);
	Look.TrunksStripe = Stream.RandRange(0, 5);
	Look.ShoeColor = Stream.RandRange(0, 5);
	Look.WrapColor = Stream.RandRange(0, 5);
	Look.GloveStyle = Stream.RandRange(0, 2);
	Look.TrunksStyle = Stream.RandRange(0, 2);
	Look.ShoeStyle = Stream.RandRange(0, 1);
	Look.WrapStyle = Stream.RandRange(0, 2);
	Look.Accessory = Stream.RandRange(0, 3);
	Look.TattooPlace = Stream.RandRange(0, 3);
	Look.Tattoos = Look.TattooPlace == 0 ? 0.f : Stream.FRand();
	Look.Scar = Stream.RandRange(0, 3);
	Sanitize(Look);
}

FFBVisualProportions FFBBoxerCustomization::ProportionsFor(const FFBPresentation& Look)
{
	FFBPresentation Clean = Look;
	if (Clean.LookVersion <= ProfileVersion)
	{
		Sanitize(Clean);
	}

	const float Muscle = FMath::Max(Clean.Muscle, Clean.Muscularity);
	const float HeightMix = Clamp01(Clean.Height * 0.55f + Clean.TorsoLength * 0.25f + Clean.LegLength * 0.20f);
	const float ArmMix = Clamp01(Clean.ArmLength * 0.65f + Clean.Reach * 0.35f);
	const float LegMix = Clamp01(Clean.LegLength * 0.70f + Clean.Height * 0.30f);
	const float ChestMix = Clamp01(Clean.Chest * 0.50f + Muscle * 0.30f + Clean.BodyFat * 0.20f);
	const float WaistMix = Clamp01(Clean.Waist * 0.45f + Clean.BodyFat * 0.40f + Clean.Hip * 0.15f);

	FFBVisualProportions Out;
	Out.SpineLength = Around(HeightMix, LengthHalfSpan);
	Out.ShoulderLength = Around(Clean.ShoulderWidth, LengthHalfSpan);
	Out.ShoulderBulk = Around(Clean.ShoulderWidth, BulkHalfSpan);
	Out.ChestBulk = Around(ChestMix, BulkHalfSpan);
	Out.WaistBulk = Around(WaistMix, BulkHalfSpan);
	Out.UpperArmLength = Around(ArmMix, LengthHalfSpan);
	Out.UpperArmBulk = Around(Clamp01(Clean.Bicep * 0.6f + Muscle * 0.4f), BulkHalfSpan);
	Out.ForearmBulk = Around(Clamp01(Clean.Bicep * 0.4f + Muscle * 0.4f + Clean.BodyFat * 0.2f), 0.08f);
	Out.ThighLength = Around(LegMix, LengthHalfSpan);
	Out.ThighBulk = Around(Clamp01(Clean.Thigh * 0.65f + Muscle * 0.20f + Clean.BodyFat * 0.15f), BulkHalfSpan);
	Out.CalfBulk = Around(Clamp01(Clean.Thigh * 0.5f + Clean.BodyFat * 0.5f), 0.08f);
	Out.NeckBulk = Around(Clean.Neck, 0.12f);
	Out.HeadWidth = Around(Clean.HeadWidth, 0.08f);
	Out.HeadLength = Around(Clean.HeadLength, 0.06f);
	const float GloveStyle = Clean.GloveStyle == 0 ? 0.94f : (Clean.GloveStyle == 2 ? 1.04f : 1.f);
	Out.GloveBulk = Around(Muscle, 0.06f) * GloveStyle;
	Out.BrowOffset = SignedCm(Clean.Brow, 2.f);
	Out.JawOffset = SignedCm(Clean.Jaw, 1.5f);
	Out.NoseOffset = SignedCm(Clean.Nose, 1.5f);
	Out.CheekOffset = SignedCm(Clean.Cheeks, 1.25f);
	Out.ChinOffset = SignedCm(Clean.Chin, 1.25f);
	Out.EyeOffset = SignedCm(Clean.Eyes, 1.f);
	Out.LipOffset = SignedCm(Clean.Lips, 0.75f);
	Out.EarOffset = SignedCm(Clean.Ears, 1.f);
	Out.Asymmetry = SignedCm(Clean.Asymmetry, 2.f);
	return Out;
}

FFBDamageLook FFBBoxerCustomization::DamageFromFight(float HeadAlpha, float BodyAlpha, float Cut, float Bruise, float Sweat)
{
	FFBDamageLook Out;
	Out.Sweat = Clamp01(Sweat);
	Out.Swelling = Clamp01(HeadAlpha);
	Out.Bruise = Clamp01(FMath::Max(Bruise, HeadAlpha * 0.85f));
	Out.BodyBruise = Clamp01(FMath::Max(BodyAlpha, Bruise * 0.5f));
	Out.Cut = Clamp01(FMath::Max(Cut, HeadAlpha > 0.55f ? (HeadAlpha - 0.55f) / 0.45f : 0.f));
	Out.Blood = Clamp01(Out.Cut * 0.85f);
	Out.Roughness = FMath::Clamp(0.48f - Out.Sweat * 0.28f + Out.Bruise * 0.05f, 0.12f, 0.85f);
	Out.Subsurface = FMath::Clamp(0.32f + Out.Swelling * 0.16f, 0.f, 1.f);
	return Out;
}

void FFBBoxerCustomization::HeightBandCm(EFBWeightClassId ClassId, float& OutMin, float& OutMax)
{
	switch (ClassId)
	{
	case EFBWeightClassId::Lightweight:
		OutMin = 160.f;
		OutMax = 175.f;
		break;
	case EFBWeightClassId::Heavyweight:
		OutMin = 180.f;
		OutMax = 198.f;
		break;
	default:
		OutMin = 170.f;
		OutMax = 185.f;
		break;
	}
}

void FFBBoxerCustomization::ReachBandCm(EFBWeightClassId ClassId, float& OutMin, float& OutMax)
{
	switch (ClassId)
	{
	case EFBWeightClassId::Lightweight:
		OutMin = 165.f;
		OutMax = 182.f;
		break;
	case EFBWeightClassId::Heavyweight:
		OutMin = 188.f;
		OutMax = 210.f;
		break;
	default:
		OutMin = 175.f;
		OutMax = 192.f;
		break;
	}
}

float FFBBoxerCustomization::DisplayHeightCm(const FFBPresentation& Look, EFBWeightClassId ClassId)
{
	float MinCm = 0.f;
	float MaxCm = 0.f;
	HeightBandCm(ClassId, MinCm, MaxCm);
	return FMath::Lerp(MinCm, MaxCm, Clamp01(Look.Height));
}

float FFBBoxerCustomization::DisplayReachCm(const FFBPresentation& Look, EFBWeightClassId ClassId)
{
	float MinCm = 0.f;
	float MaxCm = 0.f;
	ReachBandCm(ClassId, MinCm, MaxCm);
	return FMath::Lerp(MinCm, MaxCm, Clamp01(Look.Reach));
}

const TCHAR* FFBBoxerCustomization::GloveStyleName(int32 Style)
{
	switch (Style)
	{
	case 0: return TEXT("Training");
	case 2: return TEXT("Lace-up");
	default: return TEXT("Fight");
	}
}

const TCHAR* FFBBoxerCustomization::TrunksStyleName(int32 Style)
{
	switch (Style)
	{
	case 1: return TEXT("Short");
	case 2: return TEXT("Long");
	default: return TEXT("Classic");
	}
}

const TCHAR* FFBBoxerCustomization::ShoeStyleName(int32 Style)
{
	return Style == 1 ? TEXT("Low") : TEXT("High-top");
}

const TCHAR* FFBBoxerCustomization::WrapStyleName(int32 Style)
{
	switch (Style)
	{
	case 1: return TEXT("Both hands");
	case 2: return TEXT("Mexican");
	default: return TEXT("None");
	}
}

const TCHAR* FFBBoxerCustomization::AccessoryName(int32 Style)
{
	switch (Style)
	{
	case 1: return TEXT("Mouthguard");
	case 2: return TEXT("Tape");
	case 3: return TEXT("Waistband");
	default: return TEXT("None");
	}
}

bool FFBBoxerCustomization::LengthsStayReachable(const FFBVisualProportions& Proportions)
{
	const float Min = 1.f - LengthHalfSpan - 0.001f;
	const float Max = 1.f + LengthHalfSpan + 0.001f;
	auto Inside = [&](float Value)
	{
		return Value >= Min && Value <= Max;
	};
	return Inside(Proportions.SpineLength)
		&& Inside(Proportions.ShoulderLength)
		&& Inside(Proportions.UpperArmLength)
		&& Inside(Proportions.ThighLength);
}

bool FFBBoxerCustomization::BulksStayOnSkeleton(const FFBVisualProportions& Proportions)
{
	auto Inside = [](float Value, float HalfSpan)
	{
		return Value >= 1.f - HalfSpan - 0.02f && Value <= 1.f + HalfSpan + 0.02f;
	};
	return Inside(Proportions.ChestBulk, BulkHalfSpan)
		&& Inside(Proportions.WaistBulk, BulkHalfSpan)
		&& Inside(Proportions.UpperArmBulk, BulkHalfSpan)
		&& Inside(Proportions.ThighBulk, BulkHalfSpan)
		&& Inside(Proportions.NeckBulk, 0.12f)
		&& Inside(Proportions.HeadWidth, 0.08f)
		&& Proportions.GloveBulk >= 0.85f
		&& Proportions.GloveBulk <= 1.15f;
}
