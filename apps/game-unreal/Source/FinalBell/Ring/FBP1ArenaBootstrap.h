// Final Bell — procedural P1 ring (floor, lights, bounds, opponent, camera)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FBP1ArenaBootstrap.generated.h"

class AActor;
class AFBBoxerCharacter;
class AFBBroadcastCamera;
class AFBHeavyBag;
class AFBRingBounds;
class UWorld;

UCLASS()
class FINALBELL_API UFBP1ArenaBootstrap : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds the ring pieces that are missing.
	 * bSpawnOpponentIfMissing is false for two humans, so a second body is not added.
	 * bOldSchoolGym is true for 1v1 AI. It replaces the dark bowl with a wooden gym.
	 * The ring, the ropes, and the playable box stay the same size.
	 */
	static void EnsureArena(UWorld* World, AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent, AFBBroadcastCamera*& OutCamera, bool bSpawnOpponentIfMissing = true, bool bOldSchoolGym = false);

	/**
	 * Why: the game mode runs on the server, so a second PIE window never
	 * received the gym or the movable lights. Static lighting is off, and a
	 * stationary DirectionalLight leaves that window black.
	 * What: builds the gym or the bowl, forces lights to Movable, and points
	 * each local controller at an unowned ringside camera. Does not spawn a fighter.
	 */
	static void EnsureMatchLook(UWorld* World, bool bOldSchoolGym, AFBBoxerCharacter* Player, AFBBoxerCharacter* Opponent);

	/**
	 * Why: heavy bag training was standing inside the bout ring, so the bag read as part of the fight.
	 * What: builds a bag row north of that gym. Floor, walls, a beam, and movable lights.
	 * One bag takes punches. The others are visual. Does not move ring bounds or rebuild Ring_P1.
	 */
	static void EnsureBagRoom(UWorld* World, AFBHeavyBag*& OutActiveBag);

	/** Removes the bag-room shell and its lights. Does not destroy the bout ring. */
	static void DestroyBagRoom(UWorld* World);

private:
	static AActor* SpawnFloor(UWorld* World, bool bOldSchoolGym);
	static void SpawnLighting(UWorld* World, bool bOldSchoolGym);
	static AFBRingBounds* SpawnRing(UWorld* World);
	static AFBBoxerCharacter* SpawnOpponent(UWorld* World, const FVector& Location);
	static AFBBroadcastCamera* SpawnCamera(UWorld* World, AFBBoxerCharacter* A, AFBBoxerCharacter* B);
	static AFBBoxerCharacter* FindFirstPlayerBoxer(UWorld* World);
};
