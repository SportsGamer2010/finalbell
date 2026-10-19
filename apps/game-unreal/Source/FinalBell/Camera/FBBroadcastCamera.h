// Final Bell — ringside broadcast camera.
// Sits just outside the ropes, slightly above the canvas, and looks slightly down
// so both fighters' feet and most of the ring stay in frame.
// When two live boxers are far apart the same offset backs up. When they close,
// it comes in. The knockdown shot does not use that zoom.
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

	/**
	 * Why: a knockdown cuts from the ringside lens to the canvas, looking up at the referee.
	 * What: holds the camera on that shot until ClearRefereeView.
	 */
	void SetRefereeView(FVector EyeLocation, FVector LookAtLocation);
	void ClearRefereeView();

	UFUNCTION(BlueprintPure, Category = "FinalBell|Camera")
	float GetOrbitYaw() const { return OrbitYaw; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Camera")
	FRotator GetOrbitFlatRotation() const { return FRotator(0.f, OrbitYaw, 0.f); }

protected:
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Horizontal FOV. Wide enough that the 20 ft ring fits from ringside. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "45.0", ClampMax = "90.0"))
	float FightFOV = 62.f;

	/** Half-extent of the ring the lens must cover, including the ropes. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float RingHalfExtent = 360.f;

	/** Extra centimeters outside the fit distance so the near rope is not on the frame edge. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OutsideMargin = 160.f;

	/** Camera height above the fighters' feet. Just above the top rope. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float HeightAboveFeet = 240.f;

	/** Look height on the body. Hips keep the feet and the head in the same frame. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.25", ClampMax = "0.7"))
	float LookHeightFraction = 0.40f;

	/** How far the camera slides with the action, as a fraction of the midpoint offset. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DollyFollow = 0.35f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float DollyLimit = 160.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitYawSpeed = 110.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitPitchSpeed = 55.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MinPitch = 4.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MaxPitch = 18.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float LocationInterpSpeed = 3.2f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float RotationInterpSpeed = 4.f;

	/** When look input is idle, ease around the ring to a profile view of both fighters. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	bool bAutoAlignWhenIdle = true;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignSpeed = 1.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignIdleDelay = 0.28f;

	/**
	 * Center-to-center distance treated as a clinch (cm).
	 * Capsules meet near 75 cm. 110 cm still has the gloves between the chests.
	 */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "70.0"))
	float ZoomNearSeparation = 110.f;

	/**
	 * Center-to-center distance treated as the wide shot (cm).
	 * The playable box is ±270 cm, so the diagonal is about 760 cm.
	 */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "200.0"))
	float ZoomFarSeparation = 760.f;

	/**
	 * How fast the zoom chases the separation. 2.8 is about a 0.35 s lag.
	 * The location ease on top of this keeps the dolly from popping.
	 */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.5"))
	float ZoomInterpSpeed = 2.8f;

	/** Degrees the fight lens may tighten when the ropes stop a close dolly. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.0", ClampMax = "8.0"))
	float ZoomFOVTighten = 4.f;

	/** Degrees the fight lens may open when a wall stops a wide dolly. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float ZoomFOVWiden = 6.f;

	bool bRefereeView = false;
	FVector RefereeEye = FVector::ZeroVector;
	FVector RefereeLook = FVector::ZeroVector;
	float OrbitYaw = -90.f;
	float OrbitPitch = 8.f;
	float OrbitInputHoldTime = 0.f;
	bool bHasFramed = false;
	bool bTrainingFramed = false;
	FVector2D PendingOrbitInput = FVector2D::ZeroVector;

	TWeakObjectPtr<AFBBoxerCharacter> PlayerFighter;
	TWeakObjectPtr<AFBBoxerCharacter> OpponentFighter;

	/** Smoothed center-to-center gap. Starts wide so the first frame is not a clinch. */
	float SmoothedSeparation = 760.f;
	float SmoothedFOV = 62.f;
	bool bZoomInitialized = false;

	bool IsLiveBoxer(const AFBBoxerCharacter* Boxer) const;
	bool ResolveLivePair(AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent) const;
	float ResolveViewAspect() const;
	void UpdateSmoothedSeparation(float DeltaSeconds);
	void AutoFindFighters();
	void ComputeFrame(FVector& OutLocation, FVector& OutLookAt, float& OutFOV) const;
	void SnapToFrame();
};
