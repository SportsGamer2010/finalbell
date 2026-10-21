#include "Training/FBHeavyBag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

namespace
{
	constexpr float EngineShapeCm = 100.f;
}

AFBHeavyBag::AFBHeavyBag()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	SetRootComponent(Pivot);

	AnchorBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnchorBody"));
	AnchorBody->SetupAttachment(Pivot);
	AnchorBody->SetMobility(EComponentMobility::Movable);
	AnchorBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// A plate on the beam, not a sliver. The old chain cubes were 4.5 cm across.
	AnchorBody->SetRelativeScale3D(FVector(0.28f, 0.14f, 0.05f));

	Hang = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Hang"));
	Hang->SetupAttachment(Pivot);
	Hang->SetHiddenInGame(true);
	Hang->ConstraintInstance.ProfileInstance.bDisableCollision = true;

	BagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagMesh"));
	BagMesh->SetupAttachment(Pivot);
	BagMesh->SetMobility(EComponentMobility::Movable);
	BagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BagMesh->SetCanEverAffectNavigation(false);
	BagMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	TopCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopCap"));
	TopCap->SetupAttachment(BagMesh);
	TopCap->SetMobility(EComponentMobility::Movable);
	TopCap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BottomCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomCap"));
	BottomCap->SetupAttachment(BagMesh);
	BottomCap->SetMobility(EComponentMobility::Movable);
	BottomCap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (int32 Band = 0; Band < 3; ++Band)
	{
		UStaticMeshComponent* Stripe = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Band%d"), Band));
		Stripe->SetupAttachment(BagMesh);
		Stripe->SetMobility(EComponentMobility::Movable);
		Stripe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Bands.Add(Stripe);
	}

	for (int32 Link = 0; Link < 4; ++Link)
	{
		UStaticMeshComponent* Chain = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ChainLink%d"), Link));
		Chain->SetupAttachment(Pivot);
		Chain->SetMobility(EComponentMobility::Movable);
		Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Chain->SetRelativeScale3D(FVector(0.09f, 0.04f, 0.09f));
		ChainLinks.Add(Chain);
	}

	Tags.Add(TEXT("FB_HeavyBag"));
}

void AFBHeavyBag::SetBagRole(bool bInActive)
{
	bActiveBag = bInActive;
	if (bInActive)
	{
		Tags.AddUnique(TEXT("FB_ActiveBag"));
		Tags.Remove(TEXT("FB_NeighborBag"));
	}
	else
	{
		bActiveBag = false;
		Tags.Remove(TEXT("FB_ActiveBag"));
		Tags.AddUnique(TEXT("FB_NeighborBag"));
	}
}

bool AFBHeavyBag::IsActiveBag() const
{
	return bActiveBag || ActorHasTag(TEXT("FB_ActiveBag"));
}

void AFBHeavyBag::TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color) const
{
	if (!Mesh)
	{
		return;
	}
	UMaterialInterface* Base = Mesh->GetMaterial(0);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Base))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}

void AFBHeavyBag::ApplyLeatherBody()
{
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	const float DiameterScale = BodyDiameterCm() / EngineShapeCm;
	const float HeightScale = BodyHeightCm() / EngineShapeCm;
	const FVector BodyScale(DiameterScale, DiameterScale, HeightScale);

	if (AnchorBody && Cube)
	{
		AnchorBody->SetStaticMesh(Cube);
		AnchorBody->SetRelativeLocation(FVector::ZeroVector);
		AnchorBody->SetRelativeScale3D(FVector(0.28f, 0.14f, 0.05f));
	}

	if (BagMesh && Cylinder)
	{
		BagMesh->SetStaticMesh(Cylinder);
		// Hang point is the actor origin. The leather center sits one chain plus half the body below it.
		BagMesh->SetRelativeLocation(FVector(0.f, 0.f, -(ChainLengthCm() + BodyHeightCm() * 0.5f)));
		BagMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BagMesh->SetRelativeScale3D(BodyScale);
	}

	// Child offsets are in the bag's local space, and that space is already scaled.
	// ±50 lands on the cylinder ends. The cap scale cancels the tall body scale.
	const float CapRelativeZ = 0.18f / HeightScale;
	auto PlaceCap = [&](UStaticMeshComponent* Cap, float Sign)
	{
		if (!Cap || !Sphere)
		{
			return;
		}
		Cap->SetStaticMesh(Sphere);
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Cap->SetRelativeLocation(FVector(0.f, 0.f, Sign * 50.f));
		Cap->SetRelativeScale3D(FVector(1.02f, 1.02f, CapRelativeZ));
	};
	PlaceCap(TopCap, 1.f);
	PlaceCap(BottomCap, -1.f);

	const float BandRelativeZ = 0.045f / HeightScale;
	const float BandHeights[] = { 22.f, 0.f, -24.f };
	for (int32 Index = 0; Index < Bands.Num(); ++Index)
	{
		UStaticMeshComponent* Band = Bands[Index];
		if (!Band || !Cylinder)
		{
			continue;
		}
		Band->SetStaticMesh(Cylinder);
		Band->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		const float LocalZ = (Index < UE_ARRAY_COUNT(BandHeights)) ? BandHeights[Index] : 0.f;
		Band->SetRelativeLocation(FVector(0.f, 0.f, LocalZ));
		Band->SetRelativeScale3D(FVector(1.06f, 1.06f, BandRelativeZ));
	}
}

void AFBHeavyBag::BeginPlay()
{
	Super::BeginPlay();

	if (ActorHasTag(TEXT("FB_NeighborBag")))
	{
		bActiveBag = false;
	}
	if (ActorHasTag(TEXT("FB_ActiveBag")))
	{
		bActiveBag = true;
	}

	ApplyLeatherBody();

	const FLinearColor Leather(0.42f, 0.09f, 0.06f);
	const FLinearColor BandColor(0.16f, 0.05f, 0.04f);
	const FLinearColor Stripe(0.55f, 0.40f, 0.26f);
	TintMesh(BagMesh, Leather);
	TintMesh(TopCap, Leather);
	TintMesh(BottomCap, FLinearColor(0.28f, 0.07f, 0.05f));
	TintMesh(AnchorBody, FLinearColor(0.22f, 0.22f, 0.24f));
	for (int32 Index = 0; Index < Bands.Num(); ++Index)
	{
		TintMesh(Bands[Index], (Index == 1) ? Stripe : BandColor);
	}
	for (UStaticMeshComponent* Link : ChainLinks)
	{
		TintMesh(Link, FLinearColor(0.62f, 0.63f, 0.66f));
	}

	const bool bSimulate = bActiveBag && HasAuthority();
	if (!bSimulate)
	{
		if (BagMesh)
		{
			BagMesh->SetSimulatePhysics(false);
			BagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		LayoutChain();
		if (BagMesh)
		{
			BagMesh->UpdateBounds();
			const FBoxSphereBounds Bounds = BagMesh->CalcBounds(BagMesh->GetComponentTransform());
			UE_LOG(LogTemp, Log, TEXT("FinalBell neighbor bag diameter=%.1fcm height=%.1fcm"),
				Bounds.BoxExtent.X * 2.f, Bounds.BoxExtent.Z * 2.f);
		}
		return;
	}

	if (BagMesh)
	{
		// Overlap the pawn so a punch can find the leather. Block would shove the capsule.
		BagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BagMesh->SetCollisionObjectType(ECC_WorldDynamic);
		BagMesh->SetCollisionResponseToAllChannels(ECR_Block);
		BagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		BagMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		BagMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
		BagMesh->SetLinearDamping(0.28f);
		BagMesh->SetAngularDamping(0.42f);
		BagMesh->SetSimulatePhysics(true);
		BagMesh->SetEnableGravity(true);
		BagMesh->SetMassOverrideInKg(NAME_None, BagMassKg, true);
		// Physics body creation has dropped a non-uniform scale before. Put the leather back.
		BagMesh->SetWorldScale3D(FVector(BodyDiameterCm() / EngineShapeCm, BodyDiameterCm() / EngineShapeCm, BodyHeightCm() / EngineShapeCm));
	}

	if (Hang && BagMesh)
	{
		const FVector Top = BagMesh->GetComponentLocation() + FVector(0.f, 0.f, BodyHeightCm() * 0.5f);
		Hang->SetWorldLocation(Top);
		Hang->SetWorldRotation(FRotator::ZeroRotator);
		Hang->SetHiddenInGame(true);
		Hang->SetDisableCollision(true);
		Hang->SetLinearXLimit(LCM_Locked, 0.f);
		Hang->SetLinearYLimit(LCM_Locked, 0.f);
		Hang->SetLinearZLimit(LCM_Locked, 0.f);
		Hang->SetAngularSwing1Limit(ACM_Limited, 48.f);
		Hang->SetAngularSwing2Limit(ACM_Limited, 48.f);
		Hang->SetAngularTwistLimit(ACM_Limited, 40.f);
		Hang->SetAngularDriveMode(EAngularDriveMode::SLERP);
		Hang->SetAngularDriveParams(90.f, 14.f, 0.f);
		Hang->SetOrientationDriveSLERP(true);
		Hang->SetAngularOrientationTarget(FRotator::ZeroRotator);
		// The joint is the top of the bag. The world is the beam. The chain above it is only 34 cm.
		Hang->SetConstrainedComponents(BagMesh, NAME_None, nullptr, NAME_None);
	}

	LayoutChain();
	if (BagMesh)
	{
		BagMesh->UpdateBounds();
		const FBoxSphereBounds Bounds = BagMesh->CalcBounds(BagMesh->GetComponentTransform());
		UE_LOG(LogTemp, Warning, TEXT("FinalBell heavy bag diameter=%.1fcm height=%.1fcm chain=%.0fcm mass=%.0fkg"),
			Bounds.BoxExtent.X * 2.f, Bounds.BoxExtent.Z * 2.f, ChainLengthCm(), BagMassKg);
	}
}

FVector AFBHeavyBag::GetStrikeCenter() const
{
	return BagMesh ? BagMesh->GetComponentLocation() : GetActorLocation();
}

FVector AFBHeavyBag::SeatGloveCenter(const FVector& GloveCenter, float GlovePadCm, const FVector& FromLocation) const
{
	if (!BagMesh)
	{
		return GloveCenter;
	}

	const FVector Center = BagMesh->GetComponentLocation();
	const FVector Up = BagMesh->GetUpVector().GetSafeNormal();
	if (Up.IsNearlyZero())
	{
		return GloveCenter;
	}

	const float HalfHeight = BodyHeightCm() * 0.5f;
	const float Limit = BodyDiameterCm() * 0.5f + FMath::Max(0.f, GlovePadCm);
	const float Along = FVector::DotProduct(GloveCenter - Center, Up);
	const float ClampedAlong = FMath::Clamp(Along, -HalfHeight, HalfHeight);
	const FVector AxisPoint = Center + Up * ClampedAlong;
	const FVector FromAxis = GloveCenter - AxisPoint;
	const float Dist = FromAxis.Size();
	if (Dist >= Limit - 0.5f)
	{
		return GloveCenter;
	}

	FVector OutDir = FromAxis.GetSafeNormal();
	if (OutDir.IsNearlyZero())
	{
		OutDir = FVector::VectorPlaneProject(FromLocation - Center, Up).GetSafeNormal();
	}
	if (OutDir.IsNearlyZero())
	{
		return GloveCenter;
	}
	return AxisPoint + OutDir * Limit;
}

bool AFBHeavyBag::OverlapsPunch(const FVector& PunchCenter, float PunchRadius) const
{
	if (!BagMesh)
	{
		return false;
	}
	const FVector Center = BagMesh->GetComponentLocation();
	const FVector Up = BagMesh->GetUpVector();
	const float HalfHeight = BodyHeightCm() * 0.5f;
	const float Radius = BodyDiameterCm() * 0.5f;
	const float Along = FVector::DotProduct(PunchCenter - Center, Up);
	const float ClampedAlong = FMath::Clamp(Along, -HalfHeight, HalfHeight);
	const FVector AxisPoint = Center + Up * ClampedAlong;
	const float Radial = FVector::Dist(PunchCenter, AxisPoint);
	const float RadialGap = FMath::Max(0.f, Radial - Radius);
	const float VerticalGap = FMath::Max(0.f, FMath::Abs(Along) - HalfHeight);
	return FMath::Sqrt(RadialGap * RadialGap + VerticalGap * VerticalGap) <= PunchRadius;
}

void AFBHeavyBag::ApplyStrike(float Strength, const FVector& FromLocation)
{
	if (!IsActiveBag() || !HasAuthority() || !BagMesh || !BagMesh->IsSimulatingPhysics())
	{
		return;
	}
	const FVector Center = BagMesh->GetComponentLocation();
	const FVector Up = BagMesh->GetUpVector();
	const float HalfHeight = BodyHeightCm() * 0.5f;
	const float Radius = BodyDiameterCm() * 0.5f;
	const float Along = FMath::Clamp(FVector::DotProduct(FromLocation - Center, Up), -HalfHeight * 0.8f, HalfHeight * 0.8f);
	FVector Radial = (FromLocation - Center) - Up * FVector::DotProduct(FromLocation - Center, Up);
	if (Radial.SizeSquared() < 1.f)
	{
		Radial = FVector::CrossProduct(Up, FVector::ForwardVector);
		if (Radial.SizeSquared() < 1.f)
		{
			Radial = FVector::CrossProduct(Up, FVector::RightVector);
		}
	}
	Radial = Radial.GetSafeNormal();
	const FVector Contact = Center + Up * Along - Radial * Radius;

	FVector Push = Center - FromLocation;
	Push.Z *= 0.15f;
	Push = Push.GetSafeNormal();
	if (Push.IsNearlyZero())
	{
		Push = -Radial;
	}

	// 40 kg. A jab is a visible swing. A power shot climbs higher. The drive brings it home.
	const float Impulse = FMath::Clamp(Strength * 480.f, 3600.f, 18000.f);
	BagMesh->AddImpulseAtLocation(Push * Impulse, Contact);
	const FVector Spin = FVector::CrossProduct(Radial, Push).GetSafeNormal();
	const float Twist = FMath::Clamp(Strength * 5.f, 40.f, 180.f);
	if (!Spin.IsNearlyZero())
	{
		BagMesh->AddAngularImpulseInRadians(Spin * Twist, NAME_None, false);
	}
}

void AFBHeavyBag::LayoutChain()
{
	if (!BagMesh || ChainLinks.Num() == 0)
	{
		return;
	}
	const FVector Top = BagMesh->GetComponentLocation() + BagMesh->GetUpVector() * (BodyHeightCm() * 0.5f);
	const FVector Anchor = GetActorLocation();
	const int32 Count = ChainLinks.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Link = ChainLinks[Index];
		if (!Link)
		{
			continue;
		}
		const float T0 = static_cast<float>(Index) / static_cast<float>(Count);
		const float T1 = static_cast<float>(Index + 1) / static_cast<float>(Count);
		const FVector A = FMath::Lerp(Anchor, Top, T0);
		const FVector B = FMath::Lerp(Anchor, Top, T1);
		const FVector Dir = B - A;
		Link->SetWorldLocation((A + B) * 0.5f);
		const float LengthScale = FMath::Max(0.06f, Dir.Size() / EngineShapeCm);
		// 9 cm by 4 cm stock. Odd links turn so the run reads as a chain, not one line.
		const bool bTurned = (Index % 2) == 1;
		Link->SetWorldScale3D(bTurned ? FVector(0.04f, 0.09f, LengthScale) : FVector(0.09f, 0.04f, LengthScale));
		if (!Dir.IsNearlyZero())
		{
			Link->SetWorldRotation(FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator());
		}
	}
}

void AFBHeavyBag::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ActorHasTag(TEXT("FB_ActiveBag")))
	{
		bActiveBag = true;
	}
	else if (ActorHasTag(TEXT("FB_NeighborBag")))
	{
		bActiveBag = false;
	}

	if (bActiveBag && HasAuthority() && BagMesh && BagMesh->IsSimulatingPhysics())
	{
		const FVector Scale = BagMesh->GetComponentScale();
		const float WantX = BodyDiameterCm() / EngineShapeCm;
		const float WantZ = BodyHeightCm() / EngineShapeCm;
		if (Scale.X < WantX * 0.75f || Scale.Y < WantX * 0.75f || Scale.Z < WantZ * 0.75f)
		{
			BagMesh->SetWorldScale3D(FVector(WantX, WantX, WantZ));
			UE_LOG(LogTemp, Warning, TEXT("FinalBell heavy bag scale collapsed to %s. Restored the %.0f x %.0f cm leather body."),
				*Scale.ToString(), BodyDiameterCm(), BodyHeightCm());
		}
	}

	LayoutChain();
}
