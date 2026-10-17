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
enum class EFBAttackId : uint8
{
	None = 0,
	Jab,
	Cross
};

UENUM(BlueprintType)
enum class EFBGuardState : uint8
{
	Open = 0,
	High,
	Low
};

UENUM(BlueprintType)
enum class EFBContactResult : uint8
{
	Miss = 0,
	Blocked,
	CleanHit
};

/** Client→server intent. Never carries damage/stamina/win authority. */
USTRUCT(BlueprintType)
struct FFBCombatIntent
{
	GENERATED_BODY()

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

	/** Contract schema version string mirrored from packages/contracts. */
	UPROPERTY(BlueprintReadWrite, Category = "FinalBell|Combat")
	FString SchemaVersion = TEXT("1.0.0");
};

USTRUCT(BlueprintType)
struct FFBImpactEvent
{
	GENERATED_BODY()

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
};
