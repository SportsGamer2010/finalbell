#include "Combat/FBStylePolicy.h"
#include "Ring/FBRingBounds.h"
#include "EngineUtils.h"

namespace
{
	FFBStylePolicy Mix()
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 120.f;
		Policy.RangeMax = 175.f;
		Policy.Jab = 2.4f;
		Policy.Straight = 2.4f;
		Policy.LeadHook = 1.3f;
		Policy.RearHook = 1.3f;
		Policy.LeadUppercut = 0.6f;
		Policy.RearUppercut = 0.6f;
		Policy.BodyChance = 0.28f;
		Policy.PowerChance = 0.40f;
		Policy.SecondsBetweenPunches = 0.52f;
		Policy.RestFraction = 0.15f;
		Policy.Chase = 0.9f;
		Policy.Circle = 0.35f;
		Policy.CutRing = 0.25f;
		Policy.Shell = 0.15f;
		Policy.WaitForMiss = 0.20f;
		Policy.Slip = 0.40f;
		return Policy;
	}
}

FFBStylePolicy FFBStylePolicy::ForStyle(const FString& Style)
{
	auto Is = [&Style](const TCHAR* Name)
	{
		return Style.Equals(Name, ESearchCase::IgnoreCase);
	};

	if (Is(TEXT("Slugger")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 100.f;
		Policy.RangeMax = 145.f;
		Policy.Jab = 0.35f;
		Policy.Straight = 4.5f;
		Policy.LeadHook = 0.6f;
		Policy.RearHook = 2.2f;
		Policy.LeadUppercut = 0.3f;
		Policy.RearUppercut = 0.4f;
		Policy.BodyChance = 0.10f;
		Policy.PowerChance = 0.90f;
		Policy.SecondsBetweenPunches = 1.15f;
		Policy.RestFraction = 0.34f;
		Policy.Chase = 0.80f;
		Policy.Circle = 0.f;
		Policy.Shell = 0.f;
		Policy.WaitForMiss = 0.15f;
		Policy.Slip = 0.12f;
		return Policy;
	}
	if (Is(TEXT("Power puncher")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 100.f;
		Policy.RangeMax = 150.f;
		Policy.Jab = 0.5f;
		Policy.Straight = 3.5f;
		Policy.LeadHook = 1.2f;
		Policy.RearHook = 2.4f;
		Policy.LeadUppercut = 0.7f;
		Policy.RearUppercut = 0.9f;
		Policy.BodyChance = 0.15f;
		Policy.PowerChance = 0.85f;
		Policy.SecondsBetweenPunches = 1.00f;
		Policy.RestFraction = 0.30f;
		Policy.Chase = 0.75f;
		Policy.Circle = 0.f;
		Policy.Slip = 0.15f;
		return Policy;
	}
	if (Is(TEXT("Out-boxer")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 160.f;
		Policy.RangeMax = 198.f;
		Policy.Jab = 5.f;
		Policy.Straight = 1.2f;
		Policy.LeadHook = 0.25f;
		Policy.RearHook = 0.2f;
		Policy.BodyChance = 0.05f;
		Policy.PowerChance = 0.05f;
		Policy.SecondsBetweenPunches = 0.48f;
		Policy.RestFraction = 0.16f;
		Policy.Circle = 0.70f;
		Policy.Shell = 0.05f;
		Policy.WaitForMiss = 0.20f;
		Policy.Slip = 0.60f;
		return Policy;
	}
	if (Is(TEXT("Slick boxer")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 155.f;
		Policy.RangeMax = 198.f;
		Policy.Jab = 4.f;
		Policy.Straight = 1.4f;
		Policy.LeadHook = 0.8f;
		Policy.RearHook = 0.35f;
		Policy.BodyChance = 0.05f;
		Policy.PowerChance = 0.08f;
		Policy.SecondsBetweenPunches = 0.46f;
		Policy.RestFraction = 0.15f;
		Policy.Circle = 0.85f;
		Policy.Slip = 0.70f;
		Policy.WaitForMiss = 0.15f;
		return Policy;
	}
	if (Is(TEXT("Jab technician")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 165.f;
		Policy.RangeMax = 200.f;
		Policy.Jab = 7.f;
		Policy.Straight = 0.6f;
		Policy.LeadHook = 0.15f;
		Policy.RearHook = 0.1f;
		Policy.BodyChance = 0.f;
		Policy.PowerChance = 0.f;
		Policy.SecondsBetweenPunches = 0.42f;
		Policy.RestFraction = 0.16f;
		Policy.Circle = 0.45f;
		Policy.Slip = 0.40f;
		return Policy;
	}
	if (Is(TEXT("In-fighter")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 70.f;
		Policy.RangeMax = 112.f;
		Policy.Jab = 0.8f;
		Policy.Straight = 1.2f;
		Policy.LeadHook = 2.2f;
		Policy.RearHook = 2.0f;
		Policy.LeadUppercut = 1.4f;
		Policy.RearUppercut = 1.2f;
		Policy.BodyChance = 0.75f;
		Policy.PowerChance = 0.35f;
		Policy.SecondsBetweenPunches = 0.42f;
		Policy.RestFraction = 0.10f;
		Policy.CutRing = 1.10f;
		Policy.Shell = 0.05f;
		Policy.Slip = 0.10f;
		return Policy;
	}
	if (Is(TEXT("Swarmer")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 68.f;
		Policy.RangeMax = 108.f;
		Policy.Jab = 2.f;
		Policy.Straight = 1.f;
		Policy.LeadHook = 2.4f;
		Policy.RearHook = 2.4f;
		Policy.LeadUppercut = 1.f;
		Policy.RearUppercut = 1.f;
		Policy.BodyChance = 0.55f;
		Policy.PowerChance = 0.15f;
		Policy.SecondsBetweenPunches = 0.32f;
		Policy.RestFraction = 0.08f;
		Policy.CutRing = 1.20f;
		Policy.Shell = 0.f;
		Policy.Slip = 0.08f;
		return Policy;
	}
	if (Is(TEXT("Inside fighter")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 70.f;
		Policy.RangeMax = 115.f;
		Policy.Jab = 0.7f;
		Policy.Straight = 1.1f;
		Policy.LeadHook = 2.1f;
		Policy.RearHook = 2.0f;
		Policy.LeadUppercut = 1.3f;
		Policy.RearUppercut = 1.2f;
		Policy.BodyChance = 0.80f;
		Policy.PowerChance = 0.30f;
		Policy.SecondsBetweenPunches = 0.40f;
		Policy.RestFraction = 0.10f;
		Policy.CutRing = 1.15f;
		Policy.Shell = 0.05f;
		Policy.Slip = 0.12f;
		return Policy;
	}
	if (Is(TEXT("Brawler")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 75.f;
		Policy.RangeMax = 120.f;
		Policy.Jab = 0.4f;
		Policy.Straight = 1.5f;
		Policy.LeadHook = 2.8f;
		Policy.RearHook = 2.8f;
		Policy.LeadUppercut = 0.6f;
		Policy.RearUppercut = 0.6f;
		Policy.BodyChance = 0.25f;
		Policy.PowerChance = 0.55f;
		Policy.SecondsBetweenPunches = 0.40f;
		Policy.RestFraction = 0.12f;
		Policy.CutRing = 0.80f;
		Policy.Shell = 0.f;
		Policy.Slip = 0.05f;
		return Policy;
	}
	if (Is(TEXT("Counterpuncher")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 145.f;
		Policy.RangeMax = 190.f;
		Policy.Jab = 1.2f;
		Policy.Straight = 3.f;
		Policy.LeadHook = 1.6f;
		Policy.RearHook = 2.f;
		Policy.LeadUppercut = 0.4f;
		Policy.RearUppercut = 0.5f;
		Policy.BodyChance = 0.10f;
		Policy.PowerChance = 0.45f;
		Policy.SecondsBetweenPunches = 0.70f;
		Policy.RestFraction = 0.20f;
		Policy.Chase = 0.70f;
		Policy.Circle = 0.35f;
		Policy.Shell = 0.25f;
		Policy.WaitForMiss = 0.88f;
		Policy.Slip = 0.75f;
		return Policy;
	}
	if (Is(TEXT("High guard")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 125.f;
		Policy.RangeMax = 170.f;
		Policy.Jab = 2.f;
		Policy.Straight = 2.2f;
		Policy.LeadHook = 0.8f;
		Policy.RearHook = 0.8f;
		Policy.BodyChance = 0.10f;
		Policy.PowerChance = 0.30f;
		Policy.SecondsBetweenPunches = 0.65f;
		Policy.RestFraction = 0.18f;
		Policy.Chase = 0.40f;
		Policy.Circle = 0.10f;
		Policy.Shell = 1.f;
		Policy.WaitForMiss = 0.75f;
		Policy.Slip = 0.20f;
		return Policy;
	}
	if (Is(TEXT("Peek-a-boo")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 110.f;
		Policy.RangeMax = 160.f;
		Policy.Jab = 1.f;
		Policy.Straight = 1.5f;
		Policy.LeadHook = 2.f;
		Policy.RearHook = 1.6f;
		Policy.LeadUppercut = 1.2f;
		Policy.RearUppercut = 0.8f;
		Policy.BodyChance = 0.20f;
		Policy.PowerChance = 0.40f;
		Policy.SecondsBetweenPunches = 0.58f;
		Policy.RestFraction = 0.16f;
		Policy.Chase = 0.45f;
		Policy.Shell = 1.f;
		Policy.WaitForMiss = 0.72f;
		Policy.Slip = 0.25f;
		return Policy;
	}
	if (Is(TEXT("Southpaw")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 130.f;
		Policy.RangeMax = 175.f;
		Policy.Jab = 1.5f;
		Policy.Straight = 3.2f;
		Policy.LeadHook = 0.8f;
		Policy.RearHook = 2.4f;
		Policy.LeadUppercut = 0.3f;
		Policy.RearUppercut = 0.7f;
		Policy.BodyChance = 0.20f;
		Policy.PowerChance = 0.50f;
		Policy.SecondsBetweenPunches = 0.52f;
		Policy.RestFraction = 0.16f;
		Policy.Chase = 0.85f;
		Policy.Circle = 0.15f;
		Policy.SouthpawStep = 0.95f;
		Policy.Slip = 0.45f;
		Policy.WaitForMiss = 0.25f;
		return Policy;
	}
	if (Is(TEXT("Body puncher")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 78.f;
		Policy.RangeMax = 128.f;
		Policy.Jab = 0.6f;
		Policy.Straight = 1.4f;
		Policy.LeadHook = 2.2f;
		Policy.RearHook = 2.2f;
		Policy.LeadUppercut = 1.8f;
		Policy.RearUppercut = 1.6f;
		Policy.BodyChance = 1.f;
		Policy.PowerChance = 0.35f;
		Policy.SecondsBetweenPunches = 0.45f;
		Policy.RestFraction = 0.12f;
		Policy.CutRing = 0.90f;
		Policy.Shell = 0.08f;
		Policy.Slip = 0.15f;
		return Policy;
	}
	if (Is(TEXT("Boxer-puncher")) || Is(TEXT("Custom")) || Style.IsEmpty())
	{
		return Mix();
	}
	if (Is(TEXT("Speedster")))
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 150.f;
		Policy.RangeMax = 198.f;
		Policy.Jab = 4.5f;
		Policy.Straight = 1.6f;
		Policy.LeadHook = 2.2f;
		Policy.RearHook = 0.6f;
		Policy.LeadUppercut = 0.3f;
		Policy.RearUppercut = 0.2f;
		Policy.BodyChance = 0.12f;
		Policy.PowerChance = 0.f;
		Policy.SecondsBetweenPunches = 0.28f;
		Policy.RestFraction = 0.12f;
		Policy.Circle = 0.60f;
		Policy.Shell = 0.05f;
		Policy.WaitForMiss = 0.10f;
		Policy.Slip = 0.55f;
		return Policy;
	}
	return Mix();
}

bool FFBStylePolicy::FindRingCenter(const UWorld* World, FVector& OutCenter)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AFBRingBounds> It(World); It; ++It)
	{
		OutCenter = It->GetActorLocation();
		return true;
	}
	return false;
}

float FFBStylePolicy::RangeFit(float Distance, const FFBStylePolicy& Policy)
{
	if (Distance >= Policy.RangeMin && Distance <= Policy.RangeMax)
	{
		return 1.f;
	}
	const float Edge = Distance < Policy.RangeMin ? Policy.RangeMin : Policy.RangeMax;
	return FMath::Clamp(1.f - FMath::Abs(Distance - Edge) / 120.f, 0.f, 1.f);
}
