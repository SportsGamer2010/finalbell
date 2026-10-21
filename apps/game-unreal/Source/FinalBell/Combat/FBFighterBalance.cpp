#include "Combat/FBFighterBalance.h"
#include "Containers/Set.h"

int32 FFBFighterBalance::CostOfRating(int32 Rating)
{
	if (Rating < RatingMin || Rating > RatingMax)
	{
		return -1;
	}
	int32 Cost = 0;
	for (int32 Step = RatingMin + 1; Step <= Rating; ++Step)
	{
		if (Step <= 70) Cost += 1;
		else if (Step <= 80) Cost += 2;
		else if (Step <= 90) Cost += 3;
		else Cost += 5;
	}
	return Cost;
}

int32 FFBFighterBalance::NextStepCost(int32 Rating)
{
	if (Rating < RatingMin || Rating >= RatingMax)
	{
		return 0;
	}
	const int32 Next = Rating + 1;
	if (Next <= 70) return 1;
	if (Next <= 80) return 2;
	if (Next <= 90) return 3;
	return 5;
}

int32 FFBFighterBalance::CostOfBuild(const TArray<int32>& Ratings)
{
	if (!IsRatingList(Ratings))
	{
		return -1;
	}
	int32 Total = 0;
	for (int32 Rating : Ratings)
	{
		const int32 Step = CostOfRating(Rating);
		if (Step < 0)
		{
			return -1;
		}
		Total += Step;
	}
	return Total;
}

int32 FFBFighterBalance::RefundedCost(const TArray<int32>& From, const TArray<int32>& To)
{
	if (!IsRatingList(From) || !IsRatingList(To))
	{
		return -1;
	}
	int32 Refund = 0;
	for (int32 Index = 0; Index < AttributeCount; ++Index)
	{
		const int32 Before = CostOfRating(From[Index]);
		const int32 After = CostOfRating(To[Index]);
		if (Before < 0 || After < 0)
		{
			return -1;
		}
		if (After < Before)
		{
			Refund += Before - After;
		}
	}
	return Refund;
}

float FFBFighterBalance::Normalized(int32 Rating)
{
	return FMath::Clamp(static_cast<float>(Rating - RatingMin) / 45.f, 0.f, 1.f);
}

FFBCombatDerived FFBFighterBalance::Derive(const TArray<int32>& Ratings)
{
	FFBCombatDerived Out;
	if (!IsRatingList(Ratings))
	{
		return Out;
	}
	auto N = [&](EFBAttributeId Id) { return Normalized(Ratings[static_cast<int32>(Id)]); };
	Out.LeftHand = 1.f + 0.20f * N(EFBAttributeId::LeftHand);
	Out.RightHand = 1.f + 0.20f * N(EFBAttributeId::RightHand);
	Out.PunchSpeed = 1.f + 0.10f * N(EFBAttributeId::PunchSpeed);
	Out.Footwork = 1.f + 0.10f * N(EFBAttributeId::Footwork);
	Out.StaminaPool = 1.f + 0.20f * N(EFBAttributeId::Stamina);
	Out.EnduranceCost = 1.f - 0.15f * N(EFBAttributeId::Endurance);
	Out.StaminaRegen = 1.f + 0.15f * N(EFBAttributeId::StaminaRegen);
	Out.RoundRecovery = 1.f + 0.15f * N(EFBAttributeId::StaminaRecovery);
	Out.ConditioningFatigue = 1.f - 0.05f * N(EFBAttributeId::Conditioning);
	Out.ChinTaken = 1.f - 0.12f * N(EFBAttributeId::Chin);
	Out.BodyTaken = 1.f - 0.12f * N(EFBAttributeId::Body);
	Out.ChinDaze = 1.f + 0.05f * N(EFBAttributeId::Chin);
	Out.BodyDaze = 1.f + 0.05f * N(EFBAttributeId::Body);
	Out.GuardCapacity = 1.f + 0.15f * N(EFBAttributeId::BlockHealth);
	Out.GuardTaken = 1.f - 0.10f * N(EFBAttributeId::Guarding);
	Out.BlockRegen = 1.f + 0.15f * N(EFBAttributeId::BlockRegen);
	Out.CutMul = 1.f - 0.20f * N(EFBAttributeId::Cut);
	Out.BruiseMul = 1.f - 0.20f * N(EFBAttributeId::Bruise);
	Out.ReflexWeave = 1.f + 0.10f * N(EFBAttributeId::Reflexes);
	Out.ReflexWhiff = 1.f - 0.08f * N(EFBAttributeId::Reflexes);
	const float AccuracyN = N(EFBAttributeId::Accuracy);
	Out.AccuracyCone = 25.f + 3.f * AccuracyN;
	Out.CourageHold = 1.f - 0.05f * N(EFBAttributeId::Courage);
	Out.CourageAngle = 1.f + 0.05f * N(EFBAttributeId::Courage);
	return Out;
}

TArray<int32> FFBFighterBalance::Baseline()
{
	TArray<int32> Ratings;
	Ratings.Init(RatingMin, AttributeCount);
	return Ratings;
}

TArray<int32> FFBFighterBalance::PresetBalanced()
{
	return { 80, 80, 70, 70, 80, 70, 75, 75, 80, 80, 80, 80, 80, 70, 70, 70, 80, 75, 75 };
}

TArray<int32> FFBFighterBalance::PresetPuncher()
{
	return { 85, 80, 70, 70, 70, 75, 70, 70, 70, 70, 80, 75, 70, 70, 70, 70, 70, 90, 95 };
}

TArray<int32> FFBFighterBalance::PresetCounter()
{
	return { 70, 70, 70, 70, 80, 70, 70, 75, 90, 85, 85, 90, 75, 70, 70, 70, 75, 70, 70 };
}

TArray<int32> FFBFighterBalance::PresetPressure()
{
	return { 80, 80, 70, 70, 80, 70, 70, 70, 70, 70, 70, 70, 90, 70, 70, 80, 90, 80, 80 };
}

bool FFBFighterBalance::IsRatingList(const TArray<int32>& Ratings)
{
	if (Ratings.Num() != AttributeCount)
	{
		return false;
	}
	for (int32 Rating : Ratings)
	{
		if (Rating < RatingMin || Rating > RatingMax)
		{
			return false;
		}
	}
	return true;
}

bool FFBFighterBalance::IsDraftLegal(const TArray<int32>& Ratings)
{
	const int32 Cost = CostOfBuild(Ratings);
	return Cost >= 0 && Cost <= ReadyCost;
}

bool FFBFighterBalance::IsReady(const TArray<int32>& Ratings)
{
	return CostOfBuild(Ratings) == ReadyCost;
}

bool FFBFighterBalance::CanRetireInto(const FFBFighterRecord& Current, const TArray<int32>& Next, FString& OutWhy)
{
	if (!IsDraftLegal(Next))
	{
		OutWhy = TEXT("Ratings must stay between 50 and 95 and cost at most 600.");
		return false;
	}
	if (!Current.bFoughtOnce)
	{
		return true;
	}
	const int32 Refund = RefundedCost(Current.Ratings, Next);
	if (Refund < 0)
	{
		OutWhy = TEXT("The saved build could not be read.");
		return false;
	}
	if (Refund > Current.RetrainBudget)
	{
		OutWhy = FString::Printf(TEXT("This camp can refund %d points. That edit refunds %d."), Current.RetrainBudget, Refund);
		return false;
	}
	if (CostOfBuild(Next) != ReadyCost)
	{
		OutWhy = TEXT("A fighter who has already fought must stay at exactly 600 to be ready.");
		return false;
	}
	return true;
}

EFBWeightClassId FFBFighterBalance::ClassForDivision(EFBDivisionId Division)
{
	switch (Division)
	{
	case EFBDivisionId::Bantamweight:
	case EFBDivisionId::Featherweight:
	case EFBDivisionId::Lightweight:
		return EFBWeightClassId::Lightweight;
	case EFBDivisionId::Welterweight:
	case EFBDivisionId::Middleweight:
	case EFBDivisionId::LightHeavyweight:
		return EFBWeightClassId::Middleweight;
	default:
		return EFBWeightClassId::Heavyweight;
	}
}

FString FFBFighterBalance::ClassName(EFBWeightClassId ClassId)
{
	switch (ClassId)
	{
	case EFBWeightClassId::Lightweight: return TEXT("LIGHTWEIGHT");
	case EFBWeightClassId::Heavyweight: return TEXT("HEAVYWEIGHT");
	default: return TEXT("MIDDLEWEIGHT");
	}
}

FString FFBFighterBalance::DivisionName(EFBDivisionId Division)
{
	switch (Division)
	{
	case EFBDivisionId::Bantamweight: return TEXT("Bantamweight");
	case EFBDivisionId::Featherweight: return TEXT("Featherweight");
	case EFBDivisionId::Lightweight: return TEXT("Lightweight");
	case EFBDivisionId::Welterweight: return TEXT("Welterweight");
	case EFBDivisionId::Middleweight: return TEXT("Middleweight");
	case EFBDivisionId::LightHeavyweight: return TEXT("Light heavyweight");
	case EFBDivisionId::Cruiserweight: return TEXT("Cruiserweight");
	default: return TEXT("Heavyweight");
	}
}

FString FFBFighterBalance::AttributeName(EFBAttributeId Id)
{
	switch (Id)
	{
	case EFBAttributeId::Chin: return TEXT("Chin Resistance");
	case EFBAttributeId::Body: return TEXT("Body Resistance");
	case EFBAttributeId::Cut: return TEXT("Cut Resistance");
	case EFBAttributeId::Bruise: return TEXT("Bruise Resistance");
	case EFBAttributeId::Guarding: return TEXT("Guarding");
	case EFBAttributeId::Courage: return TEXT("Courage");
	case EFBAttributeId::BlockHealth: return TEXT("Block Health");
	case EFBAttributeId::BlockRegen: return TEXT("Block Regen");
	case EFBAttributeId::Reflexes: return TEXT("Reflexes");
	case EFBAttributeId::Footwork: return TEXT("Footwork");
	case EFBAttributeId::PunchSpeed: return TEXT("Punch Speed");
	case EFBAttributeId::Accuracy: return TEXT("Accuracy");
	case EFBAttributeId::Stamina: return TEXT("Stamina");
	case EFBAttributeId::StaminaRegen: return TEXT("Stamina Regen");
	case EFBAttributeId::StaminaRecovery: return TEXT("Stamina Recovery");
	case EFBAttributeId::Conditioning: return TEXT("Conditioning");
	case EFBAttributeId::Endurance: return TEXT("Endurance");
	case EFBAttributeId::LeftHand: return TEXT("Left Hand Power");
	case EFBAttributeId::RightHand: return TEXT("Right Hand Power");
	default: return TEXT("Attribute");
	}
}

FString FFBFighterBalance::AttributeEffect(EFBAttributeId Id)
{
	switch (Id)
	{
	case EFBAttributeId::Chin: return TEXT("Less head damage. A slightly higher daze line. Not a dodge.");
	case EFBAttributeId::Body: return TEXT("Less body damage. A slightly higher body-daze line.");
	case EFBAttributeId::Cut: return TEXT("Cuts open and worsen less often. Never immune.");
	case EFBAttributeId::Bruise: return TEXT("Bruises form and worsen less often. Never immune.");
	case EFBAttributeId::Guarding: return TEXT("A correct high guard loses less block. It does not track punches.");
	case EFBAttributeId::Courage: return TEXT("A slightly wider, shorter get-up. It does not add extra get-ups.");
	case EFBAttributeId::BlockHealth: return TEXT("A larger guard pool, separate from head and body.");
	case EFBAttributeId::BlockRegen: return TEXT("Guard returns faster after 0.8s without a blocked shot.");
	case EFBAttributeId::Reflexes: return TEXT("A slightly longer weave and a slightly shorter whiff.");
	case EFBAttributeId::Footwork: return TEXT("Up to 10% more ring speed on the same capsule.");
	case EFBAttributeId::PunchSpeed: return TEXT("Punches finish a little sooner. They still commit.");
	case EFBAttributeId::Accuracy: return TEXT("A slightly wider legal stick cone before the punch starts. Not a hit chance.");
	case EFBAttributeId::Stamina: return TEXT("A larger gas tank.");
	case EFBAttributeId::StaminaRegen: return TEXT("Faster refill after a short pause. A high guard refills, slower than hands down.");
	case EFBAttributeId::StaminaRecovery: return TEXT("More of the tired ceiling comes back between rounds.");
	case EFBAttributeId::Conditioning: return TEXT("Misses add a little less long fatigue. Not a second regen stat.");
	case EFBAttributeId::Endurance: return TEXT("Each punch costs less, down to 85% at 95.");
	case EFBAttributeId::LeftHand: return TEXT("Damage from the physical left hand. Stance does not swap this.");
	case EFBAttributeId::RightHand: return TEXT("Damage from the physical right hand.");
	default: return TEXT("");
	}
}

FLinearColor FFBFighterBalance::ComplexionColor(EFBComplexion Complexion)
{
	switch (Complexion)
	{
	case EFBComplexion::Fair: return FLinearColor(0.86f, 0.70f, 0.58f);
	case EFBComplexion::Olive: return FLinearColor(0.72f, 0.56f, 0.40f);
	case EFBComplexion::Tan: return FLinearColor(0.62f, 0.44f, 0.32f);
	case EFBComplexion::Brown: return FLinearColor(0.45f, 0.30f, 0.22f);
	case EFBComplexion::Deep: return FLinearColor(0.32f, 0.20f, 0.14f);
	default: return FLinearColor(0.22f, 0.14f, 0.10f);
	}
}

FFBFighterRecord FFBFighterBalance::MakePremade(const TCHAR* Id, const TCHAR* Name, EFBDivisionId Division, EFBStance Stance, const TArray<int32>& Ratings, const TCHAR* Style, const TCHAR* Plan, FFBPresentation Look)
{
	FFBFighterRecord Record;
	Record.Id = Id;
	Record.DisplayName = Name;
	Record.Division = Division;
	Record.WeightClass = ClassForDivision(Division);
	Record.Stance = Stance;
	Record.Ratings = Ratings;
	Record.Style = Style;
	Record.Plan = Plan;
	Record.Look = Look;
	Record.bPremade = true;
	Record.bFoughtOnce = true;
	Record.RetrainBudget = 0;
	return Record;
}

void FFBFighterBalance::GetPremades(TArray<FFBFighterRecord>& Out)
{
	Out.Reset();
	auto Look = [](EFBComplexion Skin, EFBHairStyle Hair, float Brow, float Jaw, float Nose, float Ink, float Muscle)
	{
		FFBPresentation Presentation;
		Presentation.Complexion = Skin;
		Presentation.Hair = Hair;
		Presentation.Brow = Brow;
		Presentation.Jaw = Jaw;
		Presentation.Nose = Nose;
		Presentation.Tattoos = Ink;
		Presentation.Muscle = Muscle;
		return Presentation;
	};
	// These 19-length lists are the roster in Tools/AI/scripts/check_fighter_balance.py.
	// Order is EFBAttributeId. The four presets stay as creator shortcuts and are not these builds.
	auto RatingsOf = [](std::initializer_list<int32> Values)
	{
		return TArray<int32>(Values);
	};
	Out.Add(MakePremade(TEXT("premade_hale"), TEXT("Marcus Hale"), EFBDivisionId::Heavyweight, EFBStance::Orthodox,
		RatingsOf({ 88, 84, 70, 70, 66, 84, 74, 52, 66, 52, 55, 68, 84, 70, 76, 54, 76, 90, 95 }),
		TEXT("Slugger"), TEXT("One clean rear hand. Slow feet, and the guard does not come back. Misses spend the gas."),
		Look(EFBComplexion::Deep, EFBHairStyle::Bald, 0.7f, 0.8f, 0.6f, 0.4f, 0.9f)));
	Out.Add(MakePremade(TEXT("premade_petrov"), TEXT("Ivan Petrov"), EFBDivisionId::Cruiserweight, EFBStance::Orthodox,
		RatingsOf({ 72, 88, 68, 70, 70, 84, 72, 68, 58, 56, 62, 60, 86, 70, 78, 92, 90, 74, 76 }),
		TEXT("In-fighter"), TEXT("Body work and a high tank. He walks through the fight. A mover who does not trade beats him."),
		Look(EFBComplexion::Fair, EFBHairStyle::Crop, 0.4f, 0.6f, 0.5f, 0.f, 0.75f)));
	Out.Add(MakePremade(TEXT("premade_voss"), TEXT("Leon Voss"), EFBDivisionId::LightHeavyweight, EFBStance::Southpaw,
		RatingsOf({ 73, 71, 69, 67, 86, 60, 79, 77, 93, 79, 82, 92, 55, 69, 67, 58, 56, 76, 80 }),
		TEXT("Counterpuncher"), TEXT("Makes you miss, then the reply is already there. A firefight empties him."),
		Look(EFBComplexion::Tan, EFBHairStyle::Fade, 0.55f, 0.4f, 0.45f, 0.2f, 0.55f)));
	Out.Add(MakePremade(TEXT("premade_quinn"), TEXT("Samir Quinn"), EFBDivisionId::Middleweight, EFBStance::Orthodox,
		RatingsOf({ 77, 75, 68, 70, 77, 74, 72, 64, 76, 79, 80, 84, 74, 68, 72, 66, 77, 86, 86 }),
		TEXT("Boxer-puncher"), TEXT("Hands and accuracy, and no empty tool. He loses if he gives up the center."),
		Look(EFBComplexion::Brown, EFBHairStyle::Short, 0.5f, 0.5f, 0.55f, 0.1f, 0.5f)));
	Out.Add(MakePremade(TEXT("premade_marte"), TEXT("Diego Marte"), EFBDivisionId::Welterweight, EFBStance::Southpaw,
		RatingsOf({ 54, 79, 68, 66, 70, 82, 70, 55, 70, 57, 92, 66, 88, 71, 77, 86, 90, 84, 60 }),
		TEXT("Swarmer"), TEXT("He smothers and keeps the hands going. A clean counter, or the outside, beats the soft chin."),
		Look(EFBComplexion::Olive, EFBHairStyle::Braids, 0.35f, 0.45f, 0.4f, 0.65f, 0.45f)));
	Out.Add(MakePremade(TEXT("premade_kane"), TEXT("Ellis Kane"), EFBDivisionId::Lightweight, EFBStance::Orthodox,
		RatingsOf({ 56, 58, 66, 64, 70, 69, 60, 72, 86, 94, 90, 91, 75, 83, 74, 66, 70, 81, 54 }),
		TEXT("Out-boxer"), TEXT("Long feet, a sharp jab, and no bomb. He loses if he is cut off and made to trade."),
		Look(EFBComplexion::Fair, EFBHairStyle::Crop, 0.3f, 0.35f, 0.3f, 0.f, 0.3f)));
	Out.Add(MakePremade(TEXT("premade_abebe"), TEXT("Nico Abebe"), EFBDivisionId::Featherweight, EFBStance::Orthodox,
		RatingsOf({ 84, 76, 80, 74, 94, 78, 92, 90, 74, 54, 56, 72, 70, 65, 68, 67, 68, 62, 60 }),
		TEXT("High guard"), TEXT("He shells up and makes them miss on the gloves. He loses if he has to chase or trade power."),
		Look(EFBComplexion::Deepest, EFBHairStyle::Braids, 0.6f, 0.4f, 0.7f, 0.25f, 0.35f)));
	Out.Add(MakePremade(TEXT("premade_solano"), TEXT("Rafi Solano"), EFBDivisionId::Bantamweight, EFBStance::Southpaw,
		RatingsOf({ 59, 80, 61, 55, 70, 81, 66, 57, 82, 88, 79, 86, 77, 70, 68, 69, 72, 93, 81 }),
		TEXT("Southpaw"), TEXT("The left hand is the rear hand, and he fights off the angle. He loses when he stands square in front of a puncher."),
		Look(EFBComplexion::Tan, EFBHairStyle::Fade, 0.45f, 0.3f, 0.5f, 0.15f, 0.4f)));
	const TCHAR* Assets[] = {
		TEXT("/Game/MetaHumans/MH_FB_Hale"),
		TEXT("/Game/MetaHumans/MH_FB_Petrov"),
		TEXT("/Game/MetaHumans/MH_FB_Voss"),
		TEXT("/Game/MetaHumans/MH_FB_Quinn"),
		TEXT("/Game/MetaHumans/MH_FB_Marte"),
		TEXT("/Game/MetaHumans/MH_FB_Kane"),
		TEXT("/Game/MetaHumans/MH_FB_Abebe"),
		TEXT("/Game/MetaHumans/MH_FB_Solano")
	};
	const float ToneU[] = { 0.28f, 0.82f, 0.58f, 0.4f, 0.66f, 0.84f, 0.18f, 0.55f };
	const float ToneV[] = { 0.45f, 0.35f, 0.55f, 0.5f, 0.62f, 0.3f, 0.48f, 0.58f };
	const float Heights[] = { 0.92f, 0.84f, 0.72f, 0.62f, 0.5f, 0.38f, 0.28f, 0.18f };
	for (int32 Index = 0; Index < Out.Num() && Index < 8; ++Index)
	{
		Out[Index].Look.MetaHumanAsset = Assets[Index];
		Out[Index].Look.SkinU = ToneU[Index];
		Out[Index].Look.SkinV = ToneV[Index];
		Out[Index].Look.Height = Heights[Index];
		Out[Index].Look.Muscularity = Out[Index].Look.Muscle;
		Out[Index].Look.Masculine = Index == 0 || Index == 1 ? 0.85f : 0.55f;
		Out[Index].Look.Beard = (Index == 0 || Index == 1) ? 2 : 0;
		Out[Index].Look.Nickname = Out[Index].DisplayName;
	}
	// Second premade in each division. No MetaHuman path: those meshes are not in the project.
	auto FinishLook = [](FFBFighterRecord& Record, float SkinU, float SkinV, float Height, float Masculine, int32 Beard)
	{
		Record.Look.SkinU = SkinU;
		Record.Look.SkinV = SkinV;
		Record.Look.Height = Height;
		Record.Look.Muscularity = Record.Look.Muscle;
		Record.Look.Masculine = Masculine;
		Record.Look.Beard = Beard;
		Record.Look.Nickname = Record.DisplayName;
	};
	Out.Add(MakePremade(TEXT("premade_drummond"), TEXT("Caleb Drummond"), EFBDivisionId::Heavyweight, EFBStance::Orthodox,
		RatingsOf({ 90, 90, 74, 74, 74, 93, 74, 73, 73, 51, 73, 52, 73, 73, 73, 73, 73, 73, 73 }),
		TEXT("Brawler"), TEXT("He walks through the fire and swings. He loses when the other man is gone and the punches hit air."),
		Look(EFBComplexion::Brown, EFBHairStyle::Bald, 0.75f, 0.85f, 0.65f, 0.55f, 0.88f)));
	Out.Add(MakePremade(TEXT("premade_crowe"), TEXT("Idris Crowe"), EFBDivisionId::Cruiserweight, EFBStance::Orthodox,
		RatingsOf({ 74, 51, 74, 74, 74, 73, 73, 73, 93, 73, 73, 90, 73, 73, 73, 52, 73, 90, 73 }),
		TEXT("Jab technician"), TEXT("The jab is the whole fight. He loses when he has to stay in the pocket and work the body."),
		Look(EFBComplexion::Deep, EFBHairStyle::Short, 0.4f, 0.55f, 0.35f, 0.1f, 0.6f)));
	Out.Add(MakePremade(TEXT("premade_ruiz"), TEXT("Mateo Ruiz"), EFBDivisionId::LightHeavyweight, EFBStance::Orthodox,
		RatingsOf({ 93, 74, 74, 74, 74, 73, 73, 73, 73, 52, 73, 73, 51, 73, 73, 73, 73, 90, 90 }),
		TEXT("Power puncher"), TEXT("Both hands are heavy and the chin holds. The gas and the feet run out if the fight goes long."),
		Look(EFBComplexion::Olive, EFBHairStyle::Crop, 0.5f, 0.6f, 0.5f, 0.3f, 0.7f)));
	Out.Add(MakePremade(TEXT("premade_pell"), TEXT("Andre Pell"), EFBDivisionId::Middleweight, EFBStance::Southpaw,
		RatingsOf({ 51, 90, 74, 74, 74, 74, 73, 73, 52, 73, 73, 73, 73, 73, 73, 90, 93, 73, 73 }),
		TEXT("Body puncher"), TEXT("Everything is to the body. A clean counter upstairs, or a mover, beats the soft chin."),
		Look(EFBComplexion::Fair, EFBHairStyle::Fade, 0.35f, 0.4f, 0.45f, 0.05f, 0.5f)));
	Out.Add(MakePremade(TEXT("premade_adel"), TEXT("Chris Adel"), EFBDivisionId::Welterweight, EFBStance::Orthodox,
		RatingsOf({ 74, 52, 74, 74, 74, 73, 73, 73, 90, 90, 73, 93, 73, 73, 73, 73, 73, 73, 51 }),
		TEXT("Slick boxer"), TEXT("He makes the ring big and the punches land. He has no bomb and no body work."),
		Look(EFBComplexion::Tan, EFBHairStyle::Short, 0.25f, 0.3f, 0.35f, 0.f, 0.35f)));
	Out.Add(MakePremade(TEXT("premade_ortega"), TEXT("Luis Ortega"), EFBDivisionId::Lightweight, EFBStance::Southpaw,
		RatingsOf({ 74, 90, 74, 74, 74, 73, 73, 73, 52, 51, 73, 73, 73, 73, 73, 90, 93, 73, 73 }),
		TEXT("Inside fighter"), TEXT("He cuts the ring off and stays on the body. Long feet and a counter beat him."),
		Look(EFBComplexion::Olive, EFBHairStyle::Braids, 0.45f, 0.4f, 0.4f, 0.4f, 0.42f)));
	Out.Add(MakePremade(TEXT("premade_sato"), TEXT("Kenji Sato"), EFBDivisionId::Featherweight, EFBStance::Orthodox,
		RatingsOf({ 51, 74, 74, 74, 74, 73, 73, 73, 93, 90, 90, 73, 73, 73, 73, 73, 73, 73, 52 }),
		TEXT("Speedster"), TEXT("Hands and feet, and he is gone. He loses if he is hit clean or made to trade the right hand."),
		Look(EFBComplexion::Fair, EFBHairStyle::Crop, 0.2f, 0.25f, 0.3f, 0.f, 0.28f)));
	Out.Add(MakePremade(TEXT("premade_nemec"), TEXT("Pavel Nemec"), EFBDivisionId::Bantamweight, EFBStance::Orthodox,
		RatingsOf({ 74, 74, 74, 74, 90, 93, 90, 73, 73, 51, 73, 52, 73, 73, 73, 73, 73, 73, 73 }),
		TEXT("Peek-a-boo"), TEXT("Shell, courage, and a high block. He loses when he has to chase or pick punches."),
		Look(EFBComplexion::Fair, EFBHairStyle::Crop, 0.55f, 0.35f, 0.4f, 0.15f, 0.32f)));
	FinishLook(Out[8], 0.34f, 0.42f, 0.9f, 0.8f, 2);
	FinishLook(Out[9], 0.22f, 0.5f, 0.8f, 0.7f, 1);
	FinishLook(Out[10], 0.62f, 0.48f, 0.74f, 0.65f, 1);
	FinishLook(Out[11], 0.78f, 0.36f, 0.6f, 0.5f, 0);
	FinishLook(Out[12], 0.58f, 0.52f, 0.48f, 0.45f, 0);
	FinishLook(Out[13], 0.7f, 0.58f, 0.36f, 0.4f, 0);
	FinishLook(Out[14], 0.86f, 0.28f, 0.26f, 0.35f, 0);
	FinishLook(Out[15], 0.8f, 0.32f, 0.16f, 0.4f, 0);
}

bool FFBFighterBalance::SelfCheck(FString& OutError)
{
	if (CostOfRating(50) != 0 || CostOfRating(60) != 10 || CostOfRating(70) != 20 || CostOfRating(75) != 30
		|| CostOfRating(80) != 40 || CostOfRating(85) != 55 || CostOfRating(90) != 70 || CostOfRating(95) != 95)
	{
		OutError = TEXT("Cost curve does not match CompetitiveFair.");
		return false;
	}
	if (CostOfRating(49) != -1 || CostOfRating(96) != -1)
	{
		OutError = TEXT("Out-of-range ratings must be rejected.");
		return false;
	}
	const TArray<int32> Presets[] = { PresetBalanced(), PresetPuncher(), PresetCounter(), PresetPressure() };
	for (const TArray<int32>& Preset : Presets)
	{
		if (CostOfBuild(Preset) != ReadyCost)
		{
			OutError = FString::Printf(TEXT("A starter preset costs %d, not 600."), CostOfBuild(Preset));
			return false;
		}
	}
	TArray<int32> All80;
	All80.Init(80, AttributeCount);
	if (CostOfBuild(All80) != 760)
	{
		OutError = TEXT("Nineteen ratings at 80 must cost 760.");
		return false;
	}
	const FFBCombatDerived Low = Derive(Baseline());
	const FFBCombatDerived High = Derive([&]() { TArray<int32> Top; Top.Init(95, AttributeCount); return Top; }());
	if (!FMath::IsNearlyEqual(Low.LeftHand, 1.f) || !FMath::IsNearlyEqual(High.LeftHand, 1.20f))
	{
		OutError = TEXT("Hand-power curve is wrong at 50 or 95.");
		return false;
	}
	if (!FMath::IsNearlyEqual(High.ChinTaken, 0.88f) || !FMath::IsNearlyEqual(High.EnduranceCost, 0.85f))
	{
		OutError = TEXT("Resistance or endurance curve is wrong at 95.");
		return false;
	}
	TArray<FFBFighterRecord> Premades;
	GetPremades(Premades);
	if (Premades.Num() != 16)
	{
		OutError = TEXT("Expected 16 division fighters.");
		return false;
	}
	TSet<FString> Styles;
	TMap<int32, int32> PerDivision;
	TMap<int32, FString> StyleInDivision;
	for (const FFBFighterRecord& Fighter : Premades)
	{
		if (!IsReady(Fighter.Ratings) || Fighter.WeightClass != ClassForDivision(Fighter.Division))
		{
			OutError = FString::Printf(TEXT("%s is not a ready build in the matching class."), *Fighter.DisplayName);
			return false;
		}
		if (Fighter.Style.IsEmpty() || Styles.Contains(Fighter.Style))
		{
			OutError = FString::Printf(TEXT("%s needs a style that no other premade uses."), *Fighter.DisplayName);
			return false;
		}
		Styles.Add(Fighter.Style);
		const int32 DivisionKey = static_cast<int32>(Fighter.Division);
		PerDivision.FindOrAdd(DivisionKey) += 1;
		if (const FString* OtherStyle = StyleInDivision.Find(DivisionKey))
		{
			if (*OtherStyle == Fighter.Style)
			{
				OutError = FString::Printf(TEXT("%s repeats a style already used in that division."), *Fighter.DisplayName);
				return false;
			}
		}
		else
		{
			StyleInDivision.Add(DivisionKey, Fighter.Style);
		}
	}
	for (int32 Division = 0; Division < 8; ++Division)
	{
		if (PerDivision.FindRef(Division) != 2)
		{
			OutError = TEXT("Each division needs two premades.");
			return false;
		}
	}
	return true;
}

FString FFBFighterBalance::ExplainBout(int32 Clean, int32 Misses, int32 Blocks, int32 BodyHits, int32 GuardBreaks, int32 Downs)
{
	if (Downs > 0)
	{
		return FString::Printf(TEXT("The canvas decided it. %d knockdowns. Clean punches %d, misses %d."), Downs, Clean, Misses);
	}
	if (GuardBreaks > 0 && Clean > Blocks)
	{
		return FString::Printf(TEXT("The guard broke %d times and the clean work followed. Body shots %d."), GuardBreaks, BodyHits);
	}
	if (Misses > Clean + 2)
	{
		return FString::Printf(TEXT("Misses outran clean punches (%d to %d). The replies were the fight."), Misses, Clean);
	}
	if (BodyHits >= 4)
	{
		return FString::Printf(TEXT("Body work showed up %d times. That is the slowdown on the card."), BodyHits);
	}
	return FString::Printf(TEXT("Clean %d, blocked %d, misses %d. The card follows the clean work."), Clean, Blocks, Misses);
}
