// Final Bell — hanging heavy bag. The boxer's capsule and punch spheres are not this actor.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FBHeavyBag.generated.h"

class UPhysicsConstraintComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class FINALBELL_API AFBHeavyBag : public AActor
{
	GENERATED_BODY()

public:
	AFBHeavyBag();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/**
	 * Call before FinishSpawningActor.
	 * The active bag takes punches. A neighbor is the same leather shape with no physics.
	 */
	void SetBagRole(bool bInActive);

	bool IsActiveBag() const;

	/** Impulse at the contact point. Strength is the punch's damage. Does not move the boxer. */
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Training")
	void ApplyStrike(float Strength, const FVector& FromLocation);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Training")
	FVector GetStrikeCenter() const;

	/** True when the punch sphere overlaps the leather body. */
	bool OverlapsPunch(const FVector& PunchCenter, float PunchRadius) const;

	/** Leather cylinder, centimeters. The engine cylinder mesh is 100 cm on a side before this scale. */
	static float BodyDiameterCm() { return 46.f; }
	static float BodyHeightCm() { return 132.f; }
	static float ChainLengthCm() { return 34.f; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<UStaticMeshComponent> AnchorBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<UPhysicsConstraintComponent> Hang;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<UStaticMeshComponent> BagMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<UStaticMeshComponent> TopCap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TObjectPtr<UStaticMeshComponent> BottomCap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TArray<TObjectPtr<UStaticMeshComponent>> Bands;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Training")
	TArray<TObjectPtr<UStaticMeshComponent>> ChainLinks;

	UPROPERTY(EditAnywhere, Category = "FinalBell|Training")
	float BagMassKg = 40.f;

	bool bActiveBag = false;

	void ApplyLeatherBody();
	void LayoutChain();
	void TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color) const;
};
