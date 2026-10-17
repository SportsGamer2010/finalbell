// Final Bell — procedural P1 ring (floor, lights, bounds, opponent, camera)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FBP1ArenaBootstrap.generated.h"

class AActor;
class AFBBoxerCharacter;
class AFBBroadcastCamera;
class AFBRingBounds;
class UWorld;

UCLASS()
class FINALBELL_API UFBP1ArenaBootstrap : public UObject
{
	GENERATED_BODY()

public:
	/** Idempotent: builds missing P1 arena pieces in World. Returns player-facing opponent if spawned/found. */
	static void EnsureArena(UWorld* World, AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent, AFBBroadcastCamera*& OutCamera);

private:
	static AActor* SpawnFloor(UWorld* World);
	static void SpawnLighting(UWorld* World);
	static AFBRingBounds* SpawnRing(UWorld* World);
	static AFBBoxerCharacter* SpawnOpponent(UWorld* World, const FVector& Location);
	static AFBBroadcastCamera* SpawnCamera(UWorld* World, AFBBoxerCharacter* A, AFBBoxerCharacter* B);
	static AFBBoxerCharacter* FindFirstPlayerBoxer(UWorld* World);
};
