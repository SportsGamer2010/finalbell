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

bool AFBRingBounds::GetCornerAnchor(EFBCornerSkin Corner, float CapsuleRadius, FVector& OutLocation, FRotator& OutFacing) const
{
	if (!RingVolume)
	{
		return false;
	}

	const FVector Ext = RingVolume->GetScaledBoxExtent();
	const float Limit = FMath::Min(Ext.X, Ext.Y);
	const float Inset = FMath::Clamp(CapsuleRadius + 24.f, 36.f, Limit * 0.45f);
	const bool bRed = Corner == EFBCornerSkin::RedCorner;
	const FVector Local(
		bRed ? (Ext.X - Inset) : -(Ext.X - Inset),
		bRed ? -(Ext.Y - Inset) : (Ext.Y - Inset),
		0.f);
	OutLocation = RingVolume->GetComponentTransform().TransformPosition(Local);
	const FVector ToCenter = (GetActorLocation() - OutLocation).GetSafeNormal2D();
	OutFacing = ToCenter.IsNearlyZero() ? FRotator::ZeroRotator : ToCenter.Rotation();
	return true;
}

bool AFBRingBounds::QueryFootwork(const FVector& WorldLocation, float CapsuleRadius, float& OutClearance, FVector& OutSaferDirection) const
{
	OutClearance = 1000.f;
	OutSaferDirection = FVector::ZeroVector;
	if (!RingVolume)
	{
		return false;
	}
	const FTransform& Transform = RingVolume->GetComponentTransform();
	const FVector Local = Transform.InverseTransformPosition(WorldLocation);
	const FVector Extent = RingVolume->GetUnscaledBoxExtent();
	const FVector Scale = Transform.GetScale3D().GetAbs();
	const float ClearX = (Extent.X - FMath::Abs(Local.X)) * Scale.X;
	const float ClearY = (Extent.Y - FMath::Abs(Local.Y)) * Scale.Y;
	OutClearance = FMath::Min(ClearX, ClearY) - FMath::Max(0.f, CapsuleRadius);
	const FVector LocalIn(Local.X, Local.Y, 0.f);
	if (LocalIn.SizeSquared2D() > 25.f)
	{
		OutSaferDirection = Transform.TransformVectorNoScale(-LocalIn).GetSafeNormal2D();
	}
	return true;
}
