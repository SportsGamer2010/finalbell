// Final Bell — Undisputed Competitor-style Enhanced Input (runtime, no .uasset)
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FBRuntimeInputLibrary.generated.h"

class UInputAction;
class UInputMappingContext;

USTRUCT()
struct FFBRuntimeInputBundle
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Move = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Look = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> PunchStick = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Turn = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Jab = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Cross = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> LeadHook = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> RearHook = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> LeadUppercut = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> RearUppercut = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Block = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Weave = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> BodyMod = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> PowerMod = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Loose = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Stance = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Taunt = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> Headbutt = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> LeftThumb = nullptr;
	UPROPERTY() TObjectPtr<UInputAction> RightThumb = nullptr;
};

UCLASS()
class FINALBELL_API UFBRuntimeInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Undisputed Competitor: face or right-stick punches, R2 block, L2 weave, L1 body, R1 power. */
	static FFBRuntimeInputBundle EnsureBoxerInput(UObject* Outer);
};
