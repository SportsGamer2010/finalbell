// Final Bell — Undisputed medium camera.
// Chest height, just behind the controlled fighter and off the lead-hand side,
// so their back sits on the right of the frame and the opponent faces the lens.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FBBroadcastCamera.generated.h"

class UCameraComponent;
class AFBBoxerCharacter;

UCLASS()
class FINALBELL_API AFBBroadcastCamera : public AActor
{
	GENERATED_BODY()

public:
	AFBBroadcastCamera();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Camera")
	void SetFighters(AFBBoxerCharacter* Player, AFBBoxerCharacter* Opponent);

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Camera")
	void ForcePossessedView();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Camera")
	void ForceViewForController(APlayerController* PC);

	/** Right-stick / mouse look. X = orbit yaw, Y = pitch. */
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Camera")
	void AddOrbitInput(FVector2D Axis);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Camera")
	float GetOrbitYaw() const { return OrbitYaw; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Camera")
	FRotator GetOrbitFlatRotation() const { return FRotator(0.f, OrbitYaw, 0.f); }

protected:
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Horizontal FOV. Narrower than the engine default so heavyweights fill the frame. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "40.0", ClampMax = "90.0"))
	float FightFOV = 58.f;

	/** Centimeters behind the controlled fighter. Stays put when they separate. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float FollowDistance = 142.f;

	/**
	 * Centimeters toward the lead-hand side.
	 * Orthodox: camera on the fighter's left, so their back occupies the right of the frame.
	 */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float LeadSideOffset = 68.f;

	/** Extra backup when the fighters are inside this separation, so the pocket still reads. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float CloseSeparation = 125.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float ClosePullback = 40.f;

	/** Camera height as a fraction of the fighter's standing height. ~0.70 is upper chest. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.45", ClampMax = "0.95"))
	float ChestHeightFraction = 0.70f;

	/** Look target height on the opponent. ~0.78 is the base of the neck / high guard. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.45", ClampMax = "0.98"))
	float LookAtHeightFraction = 0.78f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitYawSpeed = 110.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitPitchSpeed = 55.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MinPitch = 0.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MaxPitch = 16.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float LocationInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float RotationInterpSpeed = 10.f;

	/** When stick is idle, ease yaw so the camera sits behind the player. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	bool bAutoAlignWhenIdle = true;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignSpeed = 2.4f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignIdleDelay = 0.28f;

	float OrbitYaw = -90.f;
	float OrbitPitch = 5.f;
	float OrbitInputHoldTime = 0.f;
	bool bHasFramed = false;
	FVector2D PendingOrbitInput = FVector2D::ZeroVector;

	TWeakObjectPtr<AFBBoxerCharacter> PlayerFighter;
	TWeakObjectPtr<AFBBoxerCharacter> OpponentFighter;

	void AutoFindFighters();
	void ComputeFrame(FVector& OutLocation, FVector& OutLookAt) const;
	void SnapToFrame();
};
