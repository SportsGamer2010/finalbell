// Final Bell — build Enhanced Input assets in memory (no .uasset required for P1)
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

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext = nullptr;

	UPROPERTY()
	TObjectPtr<UInputAction> Move = nullptr;

	UPROPERTY()
	TObjectPtr<UInputAction> Look = nullptr;

	UPROPERTY()
	TObjectPtr<UInputAction> Jab = nullptr;

	UPROPERTY()
	TObjectPtr<UInputAction> Cross = nullptr;

	UPROPERTY()
	TObjectPtr<UInputAction> Guard = nullptr;
};

UCLASS()
class FINALBELL_API UFBRuntimeInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Creates IMC + actions owned by Outer. Safe to call repeatedly; rebuilds if missing. */
	static FFBRuntimeInputBundle EnsureBoxerInput(UObject* Outer);
};
