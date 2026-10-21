// Final Bell — shared combat enums/structs for P1 authoritative intent path
#pragma once

#include "CoreMinimal.h"
#include "FBCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EFBStance : uint8
{
	Orthodox UMETA(DisplayName = "Orthodox"),
	Southpaw UMETA(DisplayName = "Southpaw")
};

UENUM(BlueprintType)
enum class EFBPlayMode : uint8
{
	Unset = 0,
	VersusAI,
	Multiplayer,
	Training,
	/** One local bout of three 3:00 rounds. Local record only. Not a ranked pool. */
	Career
};

UENUM(BlueprintType)
enum class EFBAttackId : uint8
{
	None = 0,
	Jab,
	Cross,
	LeadHook,
	RearHook,
	LeadUppercut,
	RearUppercut
};

/** A ring action that is not a scored punch. The server still decides if it lands. */
UENUM(BlueprintType)
enum class EFBRingAction : uint8
{
	None = 0,
	Push,
	Headbutt,
	LowBlow,
	StartClinch,
	Slip,
	StanceSwitch,
	LooseToggle,
	Taunt,
	/** R1 held and D-pad down. The server starts the count. Not a damage value. */
	TakeAKnee
};

UENUM(BlueprintType)
enum class EFBGuardState : uint8
{
	Open = 0,
	High,
	Low,
	Weave
};

UENUM(BlueprintType)
enum class EFBContactResult : uint8
{
	Miss = 0,
	Blocked,
	CleanHit
};

UENUM(BlueprintType)
enum class EFBAttackPhase : uint8
{
	Idle = 0,
	Startup,
	Active,
	Recovery
};

/** What the server did with one intent. A rejected punch did not start. */
UENUM(BlueprintType)
enum class EFBIntentResult : uint8
{
	Accepted = 0,
	Buffered,
	RejectedDown,
	RejectedSequence,
	RejectedSchema,
	RejectedPhase,
	RejectedEnergy,
	RejectedBreak,
	RejectedNoAuthority
};

/**
 * Where a punch was aimed.
 * Head and body are separate pools on purpose: a shot to the chin rocks balance,
 * and a shot to the ribs takes the wind. One shared bar cannot do both jobs.
 */
UENUM(BlueprintType)
enum class EFBHitRegion : uint8
{
	Head = 0 UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body")
};

UENUM(BlueprintType)
enum class EFBCornerSkin : uint8
{
	BlueCorner,
	RedCorner
};

/** Client→server intent. Never carries damage/stamina/win authority. */
USTRUCT(BlueprintType)
struct FFBCombatIntent { GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	int32 Sequence = 0;

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	float ClientTimeSeconds = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	EFBAttackId Attack = EFBAttackId::None;

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	bool bGuardHeld = false;

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	EFBGuardState Guard = EFBGuardState::Open;

	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	FString SchemaVersion = TEXT("1.0.0");

	/** Aim the same punch at the body instead of the head. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	bool bBodyTarget = false;

	/** Harder shot. Server applies the damage scale. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	bool bPowerPunch = false;

	/** Stick was inside the tight cone. A bonus, not a requirement to throw. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	bool bPrecisionCone = false;

	/** Push, foul, clinch, slip, stance, loose movement, or taunt. None for a normal punch. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	EFBRingAction RingAction = EFBRingAction::None;

	/** Left stick at the moment of a slip or a clinch. Not a damage value. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	FVector2D MoveAxis = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FFBImpactEvent { GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	int32 AttackInstanceId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBAttackId Attack = EFBAttackId::None;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBContactResult Result = EFBContactResult::Miss;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	float DamageApplied = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	float AttackerStaminaCost = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bGuardIntercepted = false;

	/** Defender slipped or rolled under the punch. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bSlipped = false;

	/** Clean or blocked contact landed during an angle-bonus window. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bAngleBonus = false;

	/** Head or body. The client uses this for the flinch. It is not a damage amount. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBHitRegion Region = EFBHitRegion::Head;
};

/** How the bout ended. None while the clock is still running. */
UENUM(BlueprintType)
enum class EFBBoutResult : uint8
{
	None = 0,
	Decision,
	Knockout
};

/** One judge's 10-point scores, added up across the scheduled rounds. */
USTRUCT(BlueprintType)
struct FFBJudgeRow { GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 BlueScore = 10;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 RedScore = 10;
};

/**
 * The card shown when the bout ends.
 * Why: three rounds that nobody fails to beat the count go to three judges.
 * Each judge's row is the sum of their 10-point rounds. A failed count is a knockout,
 * and that card says so instead of inventing round scores.
 */
USTRUCT(BlueprintType)
struct FFBOfficialCard { GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	EFBBoutResult Result = EFBBoutResult::None;

	/** UNANIMOUS DECISION, SPLIT DECISION, MAJORITY DECISION, DRAW, or KNOCKOUT. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	FString Decision;

	/** Who the card names as the winner, or DRAW. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	FString Winner;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 BlueTotal = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 RedTotal = 0;

	/** Ring generalship accumulated this bout. Higher means that corner dictated the fight. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	float BlueGeneralship = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	float RedGeneralship = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 BlueKnockdowns = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 RedKnockdowns = 0;

	/** Clean punches count 1. Blocked punches count 0.25. Same credit as the ledger. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	float BluePunches = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	float RedPunches = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	TArray<FFBJudgeRow> Judges;

	/** Point deductions the referee took during the bout. Empty when the card is clean. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	FString DeductionNote;
};
