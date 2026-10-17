#include "Ring/FBRingBounds.h"
#include "Components/BoxComponent.h"

AFBRingBounds::AFBRingBounds()
{
	PrimaryActorTick.bCanEverTick = false;
	RingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("RingVolume"));
	SetRootComponent(RingVolume);
	RingVolume->SetBoxExtent(FVector(415.f, 415.f, 200.f));
	RingVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RingVolume->SetHiddenInGame(true);
}

void AFBRingBounds::ConfigurePlayableArea(float HalfExtentX, float HalfExtentY)
{
	if (RingVolume)
	{
		RingVolume->SetBoxExtent(FVector(HalfExtentX, HalfExtentY, 200.f));
	}
}

FVector AFBRingBounds::ClampToRing(const FVector& WorldLocation) const
{
	if (!RingVolume)
	{
		return WorldLocation;
	}
	const FVector Local = RingVolume->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Ext = RingVolume->GetUnscaledBoxExtent();
	const FVector Clamped(
		FMath::Clamp(Local.X, -Ext.X, Ext.X),
		FMath::Clamp(Local.Y, -Ext.Y, Ext.Y),
		Local.Z);
	return RingVolume->GetComponentTransform().TransformPosition(Clamped);
}
