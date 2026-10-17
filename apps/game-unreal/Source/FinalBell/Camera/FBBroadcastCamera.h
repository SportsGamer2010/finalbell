// Final Bell — stable broadcast camera framing both fighters (P1)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FBBroadcastCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;
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
	void SetFighters(AFBBoxerCharacter* A, AFBBoxerCharacter* B);

protected:
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float HeightOffset = 220.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float DistancePadding = 280.f;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Camera")
	float InterpSpeed = 4.f;

	TWeakObjectPtr<AFBBoxerCharacter> FighterA;
	TWeakObjectPtr<AFBBoxerCharacter> FighterB;

	void AutoFindFighters();
};
