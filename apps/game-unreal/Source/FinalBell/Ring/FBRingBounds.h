// Final Bell — ring boundary volume (P1 placeholder)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FBRingBounds.generated.h"

class UBoxComponent;

UCLASS()
class FINALBELL_API AFBRingBounds : public AActor
{
	GENERATED_BODY()

public:
	AFBRingBounds();

	UFUNCTION(BlueprintPure, Category = "FinalBell|Ring")
	FVector ClampToRing(const FVector& WorldLocation) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Ring")
	TObjectPtr<UBoxComponent> RingVolume;
};
