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

/** Bag-room numbers shared by the shell, the training stand, and the camera. Centimeters. */
struct FFBBagRoomLayout
{
	float FloorTopZ = -90.f;
	float SouthInnerY = 1900.f;
	float NorthInnerY = 2680.f;
	float InnerHalfX = 560.f;
	float BagY = 2525.f;
	float HangZ = 162.f;
	float RowSpacing = 200.f;
	float StandOff = 175.f;
	float CameraSide = 300.f;
	float CameraBack = 290.f;
	float CameraHeight = 150.f;
	float CameraLook = 115.f;
};

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
	 * What: builds a bag row on the north wall of a room that shares the gym floor. One bag takes punches.
	 * The others are visual. Does not move ring bounds or rebuild Ring_P1.
	 */
	static void EnsureBagRoom(UWorld* World, AFBHeavyBag*& OutActiveBag);

	/** Floor, bag row, stand-off, and camera distances for the training room. */
	static FFBBagRoomLayout GetBagRoomLayout();

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
