#include "Ring/FBP1ArenaBootstrap.h"
#include "Ring/FBRingBounds.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SpotLight.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName ArenaTag(TEXT("FB_P1Arena"));
	const FName FloorTag(TEXT("FB_P1Floor"));
	const FName RingPropTag(TEXT("FB_P1RingProp"));
	const FName OpponentTag(TEXT("FB_P1Opponent"));

	/** Half-size of a 20 ft pro ring inside the ropes (cm). */
	constexpr float CanvasHalf = 305.f;
	/** Opening center-to-center gap. Both heavyweights fill a medium shot. */
	constexpr float OpeningHalfGap = 95.f;
	/** Top of canvas Z. */
	constexpr float CanvasTopZ = 0.f;

	void TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color)
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

	AStaticMeshActor* SpawnMeshActor(
		UWorld* World,
		UStaticMesh* Mesh,
		const FVector& Loc,
		const FRotator& Rot,
		const FVector& Scale,
		const FLinearColor& Color,
		bool bCollision)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Loc, Rot, Params);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->Tags.Add(ArenaTag);
		Actor->Tags.Add(RingPropTag);
		if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
		{
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetStaticMesh(Mesh);
			Comp->SetWorldScale3D(Scale);
			if (bCollision)
			{
				Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Comp->SetCollisionResponseToAllChannels(ECR_Block);
			}
			else
			{
				Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			TintMesh(Comp, Color);
		}
		return Actor;
	}

	void ClearOldRingProps(UWorld* World)
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(RingPropTag) || It->ActorHasTag(FloorTag))
			{
				ToDestroy.Add(*It);
			}
		}
		for (TActorIterator<AFBRingBounds> It(World); It; ++It)
		{
			ToDestroy.Add(*It);
		}
		for (AActor* A : ToDestroy)
		{
			if (A)
			{
				A->Destroy();
			}
		}
	}
}

AActor* UFBP1ArenaBootstrap::SpawnFloor(UWorld* World)
{
	ClearOldRingProps(World);

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Cube)
	{
		return nullptr;
	}

	const FLinearColor CanvasColor(0.84f, 0.86f, 0.90f);
	const FLinearColor ApronColor(0.08f, 0.08f, 0.09f);
	const FLinearColor UnderColor(0.18f, 0.05f, 0.05f);
	const FLinearColor RopeWhite(0.90f, 0.93f, 0.96f);
	const FLinearColor RopeIce(0.55f, 0.78f, 0.94f);
	const FLinearColor ArenaDark(0.28f, 0.32f, 0.38f);
	const FLinearColor PostMetal(0.35f, 0.35f, 0.38f);

	// Raised platform underside + canvas top (cube default = 100cm).
	const float PlatformThickness = 80.f; // cm after scale.Z * 100
	const float PlatformScaleZ = PlatformThickness / 100.f;
	const float PlatformCenterZ = CanvasTopZ - PlatformThickness * 0.5f;
	const float CanvasScaleXY = (CanvasHalf * 2.f) / 100.f;

	AStaticMeshActor* Floor = SpawnMeshActor(
		World, Cube,
		FVector(0.f, 0.f, PlatformCenterZ),
		FRotator::ZeroRotator,
		FVector(CanvasScaleXY, CanvasScaleXY, PlatformScaleZ),
		UnderColor,
		true);
	if (Floor)
	{
		Floor->Tags.Add(FloorTag);
		// Paint top-ish look: second thinner canvas pad on top.
	}

	SpawnMeshActor(
		World, Cube,
		FVector(0.f, 0.f, CanvasTopZ + 2.f),
		FRotator::ZeroRotator,
		FVector(CanvasScaleXY * 0.98f, CanvasScaleXY * 0.98f, 0.05f),
		CanvasColor,
		true);

	// Apron skirt (dark border around canvas).
	const float SkirtH = 0.35f;
	const float SkirtZ = CanvasTopZ - 15.f;
	const float Outer = CanvasHalf + 40.f;
	const float OuterScale = (Outer * 2.f) / 100.f;
	SpawnMeshActor(World, Cube, FVector(0.f, Outer, SkirtZ), FRotator::ZeroRotator,
		FVector(OuterScale, 0.2f, SkirtH), ApronColor, true);
	SpawnMeshActor(World, Cube, FVector(0.f, -Outer, SkirtZ), FRotator::ZeroRotator,
		FVector(OuterScale, 0.2f, SkirtH), ApronColor, true);
	SpawnMeshActor(World, Cube, FVector(Outer, 0.f, SkirtZ), FRotator::ZeroRotator,
		FVector(0.2f, OuterScale, SkirtH), ApronColor, true);
	SpawnMeshActor(World, Cube, FVector(-Outer, 0.f, SkirtZ), FRotator::ZeroRotator,
		FVector(0.2f, OuterScale, SkirtH), ApronColor, true);

	// Corner posts + pads (blue near -X, red near +X).
	const float PostZ = CanvasTopZ + 70.f;
	const FVector PostLocs[] = {
		FVector(CanvasHalf, CanvasHalf, PostZ),
		FVector(CanvasHalf, -CanvasHalf, PostZ),
		FVector(-CanvasHalf, CanvasHalf, PostZ),
		FVector(-CanvasHalf, -CanvasHalf, PostZ)
	};
	const FLinearColor PadColors[] = {
		FLinearColor(0.85f, 0.75f, 0.15f),
		FLinearColor(0.85f, 0.12f, 0.1f),  // red
		FLinearColor(0.12f, 0.35f, 0.9f),  // blue
		FLinearColor(0.85f, 0.75f, 0.15f)
	};
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnMeshActor(World, Cube, PostLocs[i], FRotator::ZeroRotator,
			FVector(0.28f, 0.28f, 1.55f), PostMetal, true);
		if (Cylinder)
		{
			SpawnMeshActor(World, Cylinder, PostLocs[i] + FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator,
				FVector(0.45f, 0.45f, 0.55f), PadColors[i], false);
		}
	}

	// Four rope tiers. Ice/white so they read like the broadcast ring, not primary-color bars.
	const float RopeHeights[] = {42.f, 84.f, 124.f, 162.f};
	const FLinearColor RopeColors[] = {RopeIce, RopeWhite, RopeIce, RopeWhite};
	const float RopeLenScale = (CanvasHalf * 2.f) / 100.f;
	const float RopeThick = 0.04f;
	for (int32 Tier = 0; Tier < 4; ++Tier)
	{
		const float Z = CanvasTopZ + RopeHeights[Tier];
		const FLinearColor& Col = RopeColors[Tier];
		// +Y / -Y sides (along X)
		SpawnMeshActor(World, Cube, FVector(0.f, CanvasHalf, Z), FRotator::ZeroRotator,
			FVector(RopeLenScale, RopeThick, RopeThick), Col, false);
		SpawnMeshActor(World, Cube, FVector(0.f, -CanvasHalf, Z), FRotator::ZeroRotator,
			FVector(RopeLenScale, RopeThick, RopeThick), Col, false);
		// +X / -X sides (along Y)
		SpawnMeshActor(World, Cube, FVector(CanvasHalf, 0.f, Z), FRotator::ZeroRotator,
			FVector(RopeThick, RopeLenScale, RopeThick), Col, false);
		SpawnMeshActor(World, Cube, FVector(-CanvasHalf, 0.f, Z), FRotator::ZeroRotator,
			FVector(RopeThick, RopeLenScale, RopeThick), Col, false);
	}

	// Hidden rope-line walls so fighters bounce instead of walking out.
	const float WallZ = CanvasTopZ + 80.f;
	const float WallH = 1.8f;
	const float WallT = 0.15f;
	auto SpawnHiddenWall = [&](const FVector& Loc, const FVector& Scale)
	{
		if (AStaticMeshActor* Wall = SpawnMeshActor(World, Cube, Loc, FRotator::ZeroRotator, Scale, ApronColor, true))
		{
			Wall->SetActorHiddenInGame(true);
			if (UStaticMeshComponent* Comp = Wall->GetStaticMeshComponent())
			{
				Comp->SetVisibility(false);
			}
		}
	};
	SpawnHiddenWall(FVector(0.f, CanvasHalf + 10.f, WallZ), FVector(RopeLenScale, WallT, WallH));
	SpawnHiddenWall(FVector(0.f, -(CanvasHalf + 10.f), WallZ), FVector(RopeLenScale, WallT, WallH));
	SpawnHiddenWall(FVector(CanvasHalf + 10.f, 0.f, WallZ), FVector(WallT, RopeLenScale, WallH));
	SpawnHiddenWall(FVector(-(CanvasHalf + 10.f), 0.f, WallZ), FVector(WallT, RopeLenScale, WallH));

	// Dark bowl around the ring so the lit canvas is the only bright surface.
	const float Bowl = 1800.f;
	const float BowlScale = (Bowl * 2.f) / 100.f;
	SpawnMeshActor(World, Cube, FVector(0.f, 0.f, -8.f), FRotator::ZeroRotator,
		FVector(BowlScale, BowlScale, 0.08f), ArenaDark, true);
	const float WallScaleZ = 10.f;
	const float WallCenterZ = 500.f;
	SpawnMeshActor(World, Cube, FVector(0.f, Bowl, WallCenterZ), FRotator::ZeroRotator,
		FVector(BowlScale, 0.4f, WallScaleZ), ArenaDark, false);
	SpawnMeshActor(World, Cube, FVector(0.f, -Bowl, WallCenterZ), FRotator::ZeroRotator,
		FVector(BowlScale, 0.4f, WallScaleZ), ArenaDark, false);
	SpawnMeshActor(World, Cube, FVector(Bowl, 0.f, WallCenterZ), FRotator::ZeroRotator,
		FVector(0.4f, BowlScale, WallScaleZ), ArenaDark, false);
	SpawnMeshActor(World, Cube, FVector(-Bowl, 0.f, WallCenterZ), FRotator::ZeroRotator,
		FVector(0.4f, BowlScale, WallScaleZ), ArenaDark, false);

	return Floor;
}

void UFBP1ArenaBootstrap::SpawnLighting(UWorld* World)
{
	auto TuneKey = [](UDirectionalLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetIntensity(10.f);
		Comp->SetLightColor(FLinearColor(1.f, 0.96f, 0.90f));
		Comp->SetAtmosphereSunLight(false);
		Comp->SetCastShadows(true);
	};
	auto TuneFill = [](UDirectionalLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetIntensity(6.f);
		Comp->SetLightColor(FLinearColor(0.85f, 0.90f, 1.f));
		Comp->SetAtmosphereSunLight(false);
		Comp->SetCastShadows(false);
	};

	const FName KeyTag(TEXT("FB_P1KeyLight"));
	const FName FillTag(TEXT("FB_P1FillLight"));
	bool bHasKey = false;
	bool bHasFill = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(It->GetLightComponent());
		if (It->ActorHasTag(FillTag))
		{
			bHasFill = true;
			TuneFill(Comp);
		}
		else
		{
			bHasKey = true;
			It->Tags.Add(KeyTag);
			It->SetActorRotation(FRotator(-48.f, 30.f, 0.f));
			TuneKey(Comp);
		}
	}
	if (!bHasKey)
	{
		FActorSpawnParameters Params;
		ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 900.f), FRotator(-48.f, 30.f, 0.f), Params);
		if (Light)
		{
			Light->Tags.Add(ArenaTag);
			Light->Tags.Add(KeyTag);
			TuneKey(Cast<UDirectionalLightComponent>(Light->GetLightComponent()));
		}
	}
	if (!bHasFill)
	{
		FActorSpawnParameters Params;
		ADirectionalLight* Fill = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 700.f), FRotator(-18.f, -150.f, 0.f), Params);
		if (Fill)
		{
			Fill->Tags.Add(ArenaTag);
			Fill->Tags.Add(FillTag);
			TuneFill(Cast<UDirectionalLightComponent>(Fill->GetLightComponent()));
		}
	}

	auto TuneSky = [](USkyLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetIntensity(2.2f);
		Comp->bLowerHemisphereIsBlack = false;
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->RecaptureSky();
	};

	bool bHasSky = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		bHasSky = true;
		TuneSky(It->GetLightComponent());
	}
	if (!bHasSky)
	{
		ASkyLight* Sky = World->SpawnActor<ASkyLight>();
		if (Sky)
		{
			Sky->Tags.Add(ArenaTag);
			TuneSky(Sky->GetLightComponent());
		}
	}

	auto TuneSpot = [](USpotLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetIntensity(18000.f);
		Comp->SetLightColor(FLinearColor(1.f, 0.97f, 0.92f));
		Comp->SetAttenuationRadius(2200.f);
		Comp->SetInnerConeAngle(28.f);
		Comp->SetOuterConeAngle(70.f);
		Comp->SetCastShadows(false);
	};

	bool bHasSpots = false;
	for (TActorIterator<ASpotLight> It(World); It; ++It)
	{
		if (!It->ActorHasTag(ArenaTag))
		{
			continue;
		}
		bHasSpots = true;
		TuneSpot(Cast<USpotLightComponent>(It->GetLightComponent()));
	}
	if (!bHasSpots)
	{
		const FVector SpotLocs[] = {
			FVector(180.f, 180.f, 480.f),
			FVector(180.f, -180.f, 480.f),
			FVector(-180.f, 180.f, 480.f),
			FVector(-180.f, -180.f, 480.f)
		};
		for (const FVector& Loc : SpotLocs)
		{
			const FRotator Aim = (FVector(0.f, 0.f, 110.f) - Loc).Rotation();
			FActorSpawnParameters Params;
			ASpotLight* Spot = World->SpawnActor<ASpotLight>(Loc, Aim, Params);
			if (!Spot)
			{
				continue;
			}
			Spot->Tags.Add(ArenaTag);
			TuneSpot(Cast<USpotLightComponent>(Spot->GetLightComponent()));
		}
	}

	bool bHasPoint = false;
	for (TActorIterator<APointLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(ArenaTag))
		{
			bHasPoint = true;
			if (UPointLightComponent* Comp = Cast<UPointLightComponent>(It->GetLightComponent()))
			{
				Comp->SetIntensity(6000.f);
				Comp->SetAttenuationRadius(1800.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.95f, 0.88f));
			}
		}
	}
	if (!bHasPoint)
	{
		FActorSpawnParameters Params;
		APointLight* Point = World->SpawnActor<APointLight>(FVector(0.f, 0.f, 380.f), FRotator::ZeroRotator, Params);
		if (Point)
		{
			Point->Tags.Add(ArenaTag);
			if (UPointLightComponent* Comp = Cast<UPointLightComponent>(Point->GetLightComponent()))
			{
				Comp->SetIntensity(6000.f);
				Comp->SetAttenuationRadius(1800.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.95f, 0.88f));
			}
		}
	}
}

AFBRingBounds* UFBP1ArenaBootstrap::SpawnRing(UWorld* World)
{
	for (TActorIterator<AFBRingBounds> It(World); It; ++It)
	{
		return *It;
	}

	AFBRingBounds* Ring = World->SpawnActor<AFBRingBounds>(FVector(0.f, 0.f, CanvasTopZ + 100.f), FRotator::ZeroRotator);
	if (Ring)
	{
		Ring->Tags.Add(ArenaTag);
		Ring->ConfigurePlayableArea(CanvasHalf - 35.f, CanvasHalf - 35.f);
	}
	return Ring;
}

AFBBoxerCharacter* UFBP1ArenaBootstrap::FindFirstPlayerBoxer(UWorld* World)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (AFBBoxerCharacter* Pawn = Cast<AFBBoxerCharacter>(PC->GetPawn()))
		{
			return Pawn;
		}
	}
	for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
	{
		if (!It->ActorHasTag(OpponentTag))
		{
			return *It;
		}
	}
	return nullptr;
}

AFBBoxerCharacter* UFBP1ArenaBootstrap::SpawnOpponent(UWorld* World, const FVector& Location)
{
	for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
	{
		if (It->ActorHasTag(OpponentTag))
		{
			It->ApplyBasicSkin(EFBCornerSkin::RedCorner);
			return *It;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AFBBoxerCharacter* Opponent = World->SpawnActor<AFBBoxerCharacter>(Location, FRotator(0.f, 180.f, 0.f), Params);
	if (Opponent)
	{
		Opponent->Tags.Add(OpponentTag);
		Opponent->Tags.Add(ArenaTag);
		Opponent->Stance = EFBStance::Orthodox;
		Opponent->ApplyBasicSkin(EFBCornerSkin::RedCorner);
	}
	return Opponent;
}

AFBBroadcastCamera* UFBP1ArenaBootstrap::SpawnCamera(UWorld* World, AFBBoxerCharacter* A, AFBBoxerCharacter* B)
{
	for (TActorIterator<AFBBroadcastCamera> It(World); It; ++It)
	{
		It->SetFighters(A, B);
		It->ForcePossessedView();
		return *It;
	}

	AFBBroadcastCamera* Cam = World->SpawnActor<AFBBroadcastCamera>(FVector(0.f, -280.f, 170.f), FRotator(-12.f, 90.f, 0.f));
	if (Cam)
	{
		Cam->Tags.Add(ArenaTag);
		Cam->SetFighters(A, B);
		Cam->ForcePossessedView();
	}
	return Cam;
}

void UFBP1ArenaBootstrap::EnsureArena(UWorld* World, AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent, AFBBroadcastCamera*& OutCamera, bool bSpawnOpponentIfMissing)
{
	OutPlayer = nullptr;
	OutOpponent = nullptr;
	OutCamera = nullptr;
	if (!World)
	{
		return;
	}

	SpawnFloor(World);
	SpawnLighting(World);
	SpawnRing(World);

	OutPlayer = FindFirstPlayerBoxer(World);
	const float FighterZ = CanvasTopZ + 6.f + (OutPlayer ? OutPlayer->GetStandingHalfHeight() : 112.f);
	if (OutPlayer)
	{
		OutPlayer->SetActorLocation(FVector(-OpeningHalfGap, 0.f, FighterZ));
		OutPlayer->SetActorRotation(FRotator(0.f, 0.f, 0.f));
		OutPlayer->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
	}

	if (bSpawnOpponentIfMissing)
	{
		OutOpponent = SpawnOpponent(World, FVector(OpeningHalfGap, 0.f, FighterZ));
		if (OutOpponent)
		{
			OutOpponent->ApplyBasicSkin(EFBCornerSkin::RedCorner);
		}
	}

	if (OutPlayer && OutOpponent)
	{
		const FVector ToOpp = (OutOpponent->GetActorLocation() - OutPlayer->GetActorLocation()).GetSafeNormal2D();
		if (!ToOpp.IsNearlyZero())
		{
			OutPlayer->SetActorRotation(ToOpp.Rotation());
			OutOpponent->SetActorRotation((-ToOpp).Rotation());
		}
	}

	if (bSpawnOpponentIfMissing)
	{
		OutCamera = SpawnCamera(World, OutPlayer, OutOpponent);
	}

	UE_LOG(LogTemp, Log, TEXT("FinalBell P1 arena ready (player=%s opponent=%s spawnOpp=%d)"),
		OutPlayer ? *OutPlayer->GetName() : TEXT("none"),
		OutOpponent ? *OutOpponent->GetName() : TEXT("none"),
		bSpawnOpponentIfMissing ? 1 : 0);
}
