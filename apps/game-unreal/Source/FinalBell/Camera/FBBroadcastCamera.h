// Final Bell — Fight Night / Undisputed style orbit camera
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

	/** Extra distance beyond fighter separation. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float BaseDistance = 420.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float SeparationDistanceScale = 0.85f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MinDistance = 320.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MaxDistance = 900.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float FocusHeight = 90.f;

	/** 0 = midpoint, 1 = fully on opponent (Fight Night leans slightly toward mid). */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FocusOpponentBias = 0.35f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitYawSpeed = 110.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float OrbitPitchSpeed = 55.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MinPitch = 12.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float MaxPitch = 55.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float LocationInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float RotationInterpSpeed = 12.f;

	/** When stick is idle, ease yaw so camera sits behind the player looking at opponent. */
	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	bool bAutoAlignWhenIdle = true;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignSpeed = 1.6f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float AutoAlignIdleDelay = 0.35f;

	float OrbitYaw = -90.f;
	float OrbitPitch = 22.f;
	float OrbitInputHoldTime = 0.f;
	FVector2D PendingOrbitInput = FVector2D::ZeroVector;

	TWeakObjectPtr<AFBBoxerCharacter> PlayerFighter;
	TWeakObjectPtr<AFBBoxerCharacter> OpponentFighter;

	void AutoFindFighters();
	void ResolveFocus(FVector& OutFocus, float& OutDistance) const;
};
