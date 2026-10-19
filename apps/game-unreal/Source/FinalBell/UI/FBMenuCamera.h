// Ringside camera used only by the front end. It does not replace the broadcast fight camera.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FBMenuCamera.generated.h"

class UCameraComponent;

UCLASS()
class FINALBELL_API AFBMenuCamera : public AActor
{
	GENERATED_BODY()

public:
	AFBMenuCamera();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category="FinalBell|Menu")
	TObjectPtr<UCameraComponent> Camera;

	FVector Pivot = FVector(0.f, -430.f, 155.f);
	float Elapsed = 0.f;
};
