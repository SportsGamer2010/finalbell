// Final Bell — ring boundary volume
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/FBCombatTypes.h"
#include "FBRingBounds.generated.h"

class UBoxComponent;

UCLASS()
class FINALBELL_API AFBRingBounds : public AActor
{
	GENERATED_BODY()

public:
	AFBRingBounds();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Ring")
	void ConfigurePlayableArea(float HalfExtentX, float HalfExtentY);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Ring")
	FVector ClampToRing(const FVector& WorldLocation) const;

	/**
	 * Stable corner inside the playable box, diagonally opposite, facing the ring center.
	 * The capsule radius keeps the body off the ropes. Z is the volume center; the caller sets the feet.
	 */
	bool GetCornerAnchor(EFBCornerSkin Corner, float CapsuleRadius, FVector& OutLocation, FRotator& OutFacing) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Ring")
	TObjectPtr<UBoxComponent> RingVolume;
};
