// Final Bell — CompetitiveFair fighter builds.
// One cost function, one curve, one derived profile. Saved totals are not trusted.
#pragma once

#include "CoreMinimal.h"
#include "Combat/FBCombatTypes.h"
#include "FBFighterBalance.generated.h"

/** Nineteen purchased ratings. Index order is the contract. */
UENUM(BlueprintType)
enum class EFBAttributeId : uint8
{
	Chin = 0,
	Body,
	Cut,
	Bruise,
	Guarding,
	Courage,
	BlockHealth,
	BlockRegen,
	Reflexes,
	Footwork,
	PunchSpeed,
	Accuracy,
	Stamina,
	StaminaRegen,
	StaminaRecovery,
	Conditioning,
	Endurance,
	LeftHand,
	RightHand,
	Count
};

/** Competitive pairing uses the project's three classes. Division is a label inside one of them. */
UENUM(BlueprintType)
enum class EFBWeightClassId : uint8
{
	Lightweight = 0,
	Middleweight,
	Heavyweight
};

UENUM(BlueprintType)
enum class EFBDivisionId : uint8
{
	Bantamweight = 0,
	Featherweight,
	Lightweight,
	Welterweight,
	Middleweight,
	LightHeavyweight,
	Cruiserweight,
	Heavyweight
};

/** Presentation only. These fields do not change the capsule, reach, or damage. */
UENUM(BlueprintType)
enum class EFBComplexion : uint8
{
	Fair = 0,
	Olive,
	Tan,
	Brown,
	Deep,
	Deepest
};

UENUM(BlueprintType)
enum class EFBHairStyle : uint8
{
	Bald = 0,
	Crop,
	Short,
	Fade,
	Braids
};

USTRUCT(BlueprintType)
struct FFBPresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	EFBComplexion Complexion = EFBComplexion::Tan;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	EFBHairStyle Hair = EFBHairStyle::Crop;

	/** 0 to 1. Moves cosmetic face marks. Not a hitbox. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Brow = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Jaw = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Nose = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Tattoos = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Muscle = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString Nickname;

	/** 0 feminine, 1 in between, 2 masculine. One MetaHuman body. This is the Masculine/Feminine constraint, not a second skeleton. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 SexPresentation = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString Country = TEXT("USA");

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 GloveColor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 TrunksColor = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 TrunksStripe = 4;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 ShoeColor = 2;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 WrapColor = 4;

	/** 0 none, 1 arms, 2 chest, 3 back. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 TattooPlace = 0;

	/** 0 none, 1 brow, 2 cheek, 3 nose. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Scar = 0;

	/** Editor MetaHuman asset path. Empty until that asset exists. Not a combat mesh. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString MetaHumanAsset;

	/** Body constraints from the MetaHuman body tool, stored 0 to 1. They do not scale the capsule. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Height = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Muscularity = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float BodyFat = 0.4f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Masculine = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Chest = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Waist = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Hip = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Bicep = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Thigh = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Eyes = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Cheeks = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Chin = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Lips = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Ears = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Asymmetry = 0.5f;

	/** FMetaHumanCharacterSkinProperties U and V. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float SkinU = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float SkinV = 0.5f;

	/** EMetaHumanCharacterFrecklesMask: 0 none, 1 type 1, 2 type 2, 3 type 3. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 FreckleMask = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float FreckleDensity = 0.2f;

	/** EMetaHumanCharacterEyesIrisPattern, Iris001 through Iris009. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 IrisPattern = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float IrisHue = 0.55f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float HairGray = 0.f;

	/** 0 none, 1 stubble, 2 short, 3 full. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Beard = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Mustache = 0;

	/** EMetaHumanCharacterEyelashesType. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 EyelashType = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Foundation = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float Blush = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float LipMakeup = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	float ToothLength = 0.f;

	/** Groom slot from the plugin wardrobe: Hair, Eyebrows, Beard, Mustache, Eyelashes, Peachfuzz. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString GroomSlot = TEXT("Hair");
};

USTRUCT(BlueprintType)
struct FFBFighterRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 SchemaVersion = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString RulesetId = TEXT("CompetitiveFair");

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 RulesetVersion = 1;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	EFBWeightClassId WeightClass = EFBWeightClassId::Middleweight;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	EFBDivisionId Division = EFBDivisionId::Middleweight;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	EFBStance Stance = EFBStance::Orthodox;

	/** Length 19. Values are ratings 50-95. Recomputed on load. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	TArray<int32> Ratings;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FFBPresentation Look;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString Plan;

	/** Classic style this build is shaped around. Empty on a custom until one is chosen. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	FString Style;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 CareerXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Wins = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Losses = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 Draws = 0;

	/** Refund budget after a camp. Creation before the first fight ignores this. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	int32 RetrainBudget = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	bool bFoughtOnce = false;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Fighter")
	bool bPremade = false;
};

/** Derived combat numbers. Not saved. Built only from a validated rating list. */
struct FFBCombatDerived
{
	float LeftHand = 1.f;
	float RightHand = 1.f;
	float PunchSpeed = 1.f;
	float Footwork = 1.f;
	float StaminaPool = 1.f;
	float EnduranceCost = 1.f;
	float StaminaRegen = 1.f;
	float RoundRecovery = 1.f;
	float ConditioningFatigue = 1.f;
	float ChinTaken = 1.f;
	float BodyTaken = 1.f;
	float ChinDaze = 1.f;
	float BodyDaze = 1.f;
	float GuardCapacity = 1.f;
	float GuardTaken = 1.f;
	float BlockRegen = 1.f;
	float CutMul = 1.f;
	float BruiseMul = 1.f;
	float ReflexWeave = 1.f;
	float ReflexWhiff = 1.f;
	float AccuracyCone = 26.f;
	float CourageHold = 1.f;
	float CourageAngle = 1.f;
};

class FINALBELL_API FFBFighterBalance
{
public:
	static constexpr int32 AttributeCount = 19;
	static constexpr int32 RatingMin = 50;
	static constexpr int32 RatingMax = 95;
	static constexpr int32 ReadyCost = 600;
	static constexpr int32 RetrainCap = 20;
	static constexpr int32 MaxCreated = 10;
	static constexpr int32 RulesetVersion = 1;
	static constexpr float RegenDelaySeconds = 0.6f;
	static constexpr float BlockDelaySeconds = 0.8f;
	static constexpr float ExposureCap = 1.60f;

	static int32 CostOfRating(int32 Rating);
	static int32 NextStepCost(int32 Rating);
	static int32 CostOfBuild(const TArray<int32>& Ratings);
	static int32 RefundedCost(const TArray<int32>& From, const TArray<int32>& To);
	static float Normalized(int32 Rating);
	static FFBCombatDerived Derive(const TArray<int32>& Ratings);
	static TArray<int32> Baseline();
	static TArray<int32> PresetBalanced();
	static TArray<int32> PresetPuncher();
	static TArray<int32> PresetCounter();
	static TArray<int32> PresetPressure();
	static bool IsRatingList(const TArray<int32>& Ratings);
	static bool IsDraftLegal(const TArray<int32>& Ratings);
	static bool IsReady(const TArray<int32>& Ratings);
	static bool CanRetireInto(const FFBFighterRecord& Current, const TArray<int32>& Next, FString& OutWhy);
	static EFBWeightClassId ClassForDivision(EFBDivisionId Division);
	static FString ClassName(EFBWeightClassId ClassId);
	static FString DivisionName(EFBDivisionId Division);
	static FString AttributeName(EFBAttributeId Id);
	static FString AttributeEffect(EFBAttributeId Id);
	static FLinearColor ComplexionColor(EFBComplexion Complexion);
	static void GetPremades(TArray<FFBFighterRecord>& Out);
	static FFBFighterRecord MakePremade(const TCHAR* Id, const TCHAR* Name, EFBDivisionId Division, EFBStance Stance, const TArray<int32>& Ratings, const TCHAR* Style, const TCHAR* Plan, FFBPresentation Look);
	static bool SelfCheck(FString& OutError);
	static FString ExplainBout(int32 Clean, int32 Misses, int32 Blocks, int32 BodyHits, int32 GuardBreaks, int32 Downs);
};
