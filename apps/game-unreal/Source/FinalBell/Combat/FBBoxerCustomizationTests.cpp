#include "Misc/AutomationTest.h"
#include "Combat/FBBoxerCustomization.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBCreatorProfileTest,
	"FinalBell.Creator.Profile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBCreatorProfileTest::RunTest(const FString& Parameters)
{
	FFBPresentation Look;
	FString Why;
	FFBBoxerCustomization::Sanitize(Look);
	TestTrue(TEXT("A default look is a legal middleweight"), FFBBoxerCustomization::Validate(Look, EFBWeightClassId::Middleweight, Why));
	TestEqual(TEXT("Profile version is 1"), Look.LookVersion, FFBBoxerCustomization::ProfileVersion);
	TestEqual(TEXT("Default age is 28"), Look.Age, 28);

	Look.Age = 10;
	Look.Hometown = FString::ChrN(80, TEXT('A'));
	Look.Country.Reset();
	Look.LookVersion = 0;
	FFBBoxerCustomization::Sanitize(Look);
	TestEqual(TEXT("Age clamps to 18"), Look.Age, 18);
	TestEqual(TEXT("Hometown clamps to 48"), Look.Hometown.Len(), 48);
	TestEqual(TEXT("Empty country becomes USA"), Look.Country, FString(TEXT("USA")));
	TestEqual(TEXT("Missing version becomes 1"), Look.LookVersion, 1);
	TestTrue(TEXT("Clamped look still validates"), FFBBoxerCustomization::Validate(Look, EFBWeightClassId::Lightweight, Why));

	Look.LookVersion = 99;
	TestFalse(TEXT("A newer profile is refused"), FFBBoxerCustomization::Validate(Look, EFBWeightClassId::Heavyweight, Why));

	FFBBoxerCustomization::ResetLook(Look);
	const FFBVisualProportions Neutral = FFBBoxerCustomization::ProportionsFor(Look);
	TestTrue(TEXT("Neutral spine length is 1"), FMath::IsNearlyEqual(Neutral.SpineLength, 1.f, 0.001f));
	TestTrue(TEXT("Neutral arm length is 1"), FMath::IsNearlyEqual(Neutral.UpperArmLength, 1.f, 0.001f));
	TestTrue(TEXT("Neutral lengths stay on the punch pose"), FFBBoxerCustomization::LengthsStayReachable(Neutral));

	const EFBWeightClassId Classes[] = {
		EFBWeightClassId::Lightweight,
		EFBWeightClassId::Middleweight,
		EFBWeightClassId::Heavyweight
	};
	for (const EFBWeightClassId ClassId : Classes)
	{
		for (const float Slider : { 0.f, 0.5f, 1.f })
		{
			FFBPresentation Sample;
			Sample.Height = Slider;
			Sample.Reach = Slider;
			Sample.ShoulderWidth = Slider;
			Sample.Chest = Slider;
			Sample.TorsoLength = Slider;
			Sample.ArmLength = Slider;
			Sample.LegLength = Slider;
			Sample.Muscularity = Slider;
			Sample.Muscle = Slider;
			Sample.BodyFat = Slider;
			Sample.Neck = Slider;
			Sample.HeadWidth = Slider;
			Sample.HeadLength = Slider;
			Sample.Bicep = Slider;
			Sample.Thigh = Slider;
			Sample.Waist = Slider;
			const FFBVisualProportions Built = FFBBoxerCustomization::ProportionsFor(Sample);
			TestTrue(TEXT("Length stays inside 3 percent"), FFBBoxerCustomization::LengthsStayReachable(Built));
			TestTrue(TEXT("Bulk stays on the skeleton"), FFBBoxerCustomization::BulksStayOnSkeleton(Built));
			FString ClassWhy;
			FFBBoxerCustomization::Sanitize(Sample);
			TestTrue(TEXT("Each class accepts the slider"), FFBBoxerCustomization::Validate(Sample, ClassId, ClassWhy));
		}
	}

	FFBPresentation Short;
	Short.Height = 0.f;
	FFBPresentation Tall;
	Tall.Height = 1.f;
	const float LightTall = FFBBoxerCustomization::DisplayHeightCm(Tall, EFBWeightClassId::Lightweight);
	const float HeavyShort = FFBBoxerCustomization::DisplayHeightCm(Short, EFBWeightClassId::Heavyweight);
	TestTrue(TEXT("Lightweight height stays at or under 175 cm"), LightTall <= 175.1f);
	TestTrue(TEXT("Heavyweight height starts at 180 cm"), HeavyShort >= 179.9f);
	TestTrue(TEXT("The two class bands do not overlap"), LightTall < HeavyShort);

	FFBPresentation A;
	FFBPresentation B;
	FFBPresentation C;
	FFBBoxerCustomization::Randomize(A, 42);
	FFBBoxerCustomization::Randomize(B, 42);
	FFBBoxerCustomization::Randomize(C, 99);
	TestEqual(TEXT("The same seed repeats height"), A.Height, B.Height);
	TestEqual(TEXT("The same seed repeats age"), A.Age, B.Age);
	TestEqual(TEXT("The same seed repeats hometown"), A.Hometown, B.Hometown);
	TestTrue(TEXT("A different seed changes the look"), !FMath::IsNearlyEqual(A.Height, C.Height, 0.0001f) || A.Hometown != C.Hometown || A.Age != C.Age);
	TestTrue(TEXT("Random lightweight still validates"), FFBBoxerCustomization::Validate(A, EFBWeightClassId::Lightweight, Why));
	TestTrue(TEXT("Random heavyweight still validates"), FFBBoxerCustomization::Validate(C, EFBWeightClassId::Heavyweight, Why));

	FFBPresentation Copy = A;
	Copy.Height = 0.2f;
	TestFalse(TEXT("Two profiles do not share storage"), FMath::IsNearlyEqual(A.Height, Copy.Height, 0.0001f));

	const FFBDamageLook Fresh = FFBBoxerCustomization::DamageFromFight(0.f, 0.f, 0.f, 0.f, 0.f);
	const FFBDamageLook Tired = FFBBoxerCustomization::DamageFromFight(0.7f, 0.4f, 0.5f, 0.4f, 0.8f);
	TestTrue(TEXT("A fresh fighter is not sweating"), Fresh.Sweat < 0.01f);
	TestTrue(TEXT("Damage raises sweat, bruise, cut, and blood"), Tired.Sweat > Fresh.Sweat && Tired.Bruise > Fresh.Bruise && Tired.Cut > Fresh.Cut && Tired.Blood > Fresh.Blood);
	TestTrue(TEXT("Sweat lowers roughness"), Tired.Roughness < Fresh.Roughness);
	TestTrue(TEXT("Swelling raises subsurface"), Tired.Subsurface > Fresh.Subsurface);
	return true;
}

#endif
