#include "Ring/FBP1ArenaBootstrap.h"
#include "Ring/FBRingBounds.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Training/FBHeavyBag.h"
#include "Combat/FBCombatTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
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
	const FName GymLightTag(TEXT("FB_P1GymLight"));

	/** Half-size of a 20 ft pro ring inside the ropes (cm). */
	constexpr float CanvasHalf = 305.f;
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

	void DestroyByTag(UWorld* World, FName Tag)
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				ToDestroy.Add(*It);
			}
		}
		for (AActor* Actor : ToDestroy)
		{
			if (Actor)
			{
				Actor->Destroy();
			}
		}
	}

	/**
	 * Why: 1v1 AI should feel like a neighborhood gym. The fight stays on the
	 * same 20 ft ring, so the playable box does not change.
	 * What: a wood floor under the platform, brick walls, a low ceiling, beams,
	 * dirty windows, four heavy bags, a bench, a bucket, a mirror, and two
	 * posters. The bags and walls do not block. The floor is the ground.
	 */
	void SpawnOldSchoolGym(UWorld* World, UStaticMesh* Cube, UStaticMesh* Cylinder)
	{
		if (!Cube)
		{
			return;
		}

		const FLinearColor Wood(0.36f, 0.22f, 0.12f);
		const FLinearColor WoodDark(0.24f, 0.14f, 0.08f);
		const FLinearColor Brick(0.42f, 0.20f, 0.14f);
		const FLinearColor BrickDark(0.28f, 0.13f, 0.10f);
		const FLinearColor Glass(0.72f, 0.80f, 0.74f);
		const FLinearColor Leather(0.10f, 0.07f, 0.05f);
		const FLinearColor Metal(0.45f, 0.42f, 0.38f);
		const FLinearColor Ceiling(0.10f, 0.09f, 0.08f);
		const FLinearColor PosterCream(0.78f, 0.70f, 0.52f);
		const FLinearColor PosterRed(0.55f, 0.16f, 0.13f);

		const float Room = 1400.f;
		const float RoomScale = (Room * 2.f) / 100.f;

		// The ring platform occupies the middle. This floor is the gym around it.
		SpawnMeshActor(World, Cube, FVector(0.f, 0.f, -96.f), FRotator::ZeroRotator,
			FVector(RoomScale, RoomScale, 0.12f), Wood, true);
		for (int32 Board = -4; Board <= 4; ++Board)
		{
			SpawnMeshActor(World, Cube, FVector(0.f, Board * 280.f, -88.f), FRotator::ZeroRotator,
				FVector(RoomScale * 0.98f, 0.06f, 0.02f), WoodDark, false);
		}

		const float WallZ = 260.f;
		const float WallH = 5.4f;
		// North wall is split. The gap is a hall toward the bag room, not an opening into the ring.
		SpawnMeshActor(World, Cube, FVector(-770.f, Room, WallZ), FRotator::ZeroRotator,
			FVector(12.6f, 0.45f, WallH), Brick, false);
		SpawnMeshActor(World, Cube, FVector(770.f, Room, WallZ), FRotator::ZeroRotator,
			FVector(12.6f, 0.45f, WallH), Brick, false);
		SpawnMeshActor(World, Cube, FVector(0.f, 1650.f, -96.f), FRotator::ZeroRotator,
			FVector(2.8f, 5.0f, 0.12f), Wood, true);
		SpawnMeshActor(World, Cube, FVector(0.f, -Room, WallZ), FRotator::ZeroRotator,
			FVector(RoomScale, 0.45f, WallH), BrickDark, false);
		SpawnMeshActor(World, Cube, FVector(Room, 0.f, WallZ), FRotator::ZeroRotator,
			FVector(0.45f, RoomScale, WallH), Brick, false);
		SpawnMeshActor(World, Cube, FVector(-Room, 0.f, WallZ), FRotator::ZeroRotator,
			FVector(0.45f, RoomScale, WallH), BrickDark, false);

		SpawnMeshActor(World, Cube, FVector(0.f, 0.f, 540.f), FRotator::ZeroRotator,
			FVector(RoomScale, RoomScale, 0.12f), Ceiling, false);

		const float BeamZ = 500.f;
		for (int32 Beam = -1; Beam <= 1; ++Beam)
		{
			SpawnMeshActor(World, Cube, FVector(0.f, Beam * 520.f, BeamZ), FRotator::ZeroRotator,
				FVector(RoomScale * 0.96f, 0.16f, 0.14f), WoodDark, false);
		}

		auto Window = [&](const FVector& Loc, const FVector& Scale)
		{
			SpawnMeshActor(World, Cube, Loc, FRotator::ZeroRotator, Scale, Glass, false);
		};
		Window(FVector(0.f, Room - 40.f, 360.f), FVector(2.4f, 0.08f, 0.9f));
		Window(FVector(-520.f, Room - 40.f, 360.f), FVector(1.4f, 0.08f, 0.7f));
		Window(FVector(520.f, Room - 40.f, 360.f), FVector(1.4f, 0.08f, 0.7f));
		Window(FVector(Room - 40.f, 200.f, 340.f), FVector(0.08f, 1.6f, 0.8f));
		Window(FVector(-(Room - 40.f), -180.f, 340.f), FVector(0.08f, 1.6f, 0.8f));

		if (Cylinder)
		{
			const float BagX[] = {-520.f, -180.f, 180.f, 520.f};
			for (float X : BagX)
			{
				SpawnMeshActor(World, Cylinder, FVector(X, Room - 180.f, 430.f), FRotator::ZeroRotator,
					FVector(0.06f, 0.06f, 1.1f), Metal, false);
				SpawnMeshActor(World, Cylinder, FVector(X, Room - 180.f, 210.f), FRotator::ZeroRotator,
					FVector(0.72f, 0.72f, 2.3f), Leather, false);
			}
			SpawnMeshActor(World, Cylinder, FVector(480.f, -520.f, 22.f), FRotator::ZeroRotator,
				FVector(0.32f, 0.32f, 0.36f), Metal, false);
		}

		SpawnMeshActor(World, Cube, FVector(-1100.f, 80.f, 28.f), FRotator::ZeroRotator,
			FVector(0.55f, 2.4f, 0.32f), Wood, false);
		SpawnMeshActor(World, Cube, FVector(-1100.f, 80.f, 52.f), FRotator::ZeroRotator,
			FVector(0.62f, 2.5f, 0.08f), WoodDark, false);

		SpawnMeshActor(World, Cube, FVector(-(Room - 50.f), 40.f, 170.f), FRotator::ZeroRotator,
			FVector(0.06f, 3.4f, 1.7f), FLinearColor(0.16f, 0.18f, 0.20f), false);
		SpawnMeshActor(World, Cube, FVector(Room - 50.f, 220.f, 190.f), FRotator::ZeroRotator,
			FVector(0.06f, 0.9f, 1.15f), PosterCream, false);
		SpawnMeshActor(World, Cube, FVector(Room - 50.f, -180.f, 170.f), FRotator::ZeroRotator,
			FVector(0.06f, 0.7f, 0.95f), PosterRed, false);
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

AActor* UFBP1ArenaBootstrap::SpawnFloor(UWorld* World, bool bOldSchoolGym)
{
	const FName ShellTag = bOldSchoolGym ? FName(TEXT("FB_P1GymShell")) : FName(TEXT("FB_P1BowlShell"));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(ShellTag))
		{
			return *It;
		}
	}

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
		Floor->Tags.Add(ShellTag);
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

	if (bOldSchoolGym)
	{
		SpawnOldSchoolGym(World, Cube, Cylinder);
	}
	else
	{
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
	}

	return Floor;
}

void UFBP1ArenaBootstrap::SpawnLighting(UWorld* World, bool bOldSchoolGym)
{
	const FName ReadyTag = bOldSchoolGym ? FName(TEXT("FB_P1LightsReadyGym")) : FName(TEXT("FB_P1LightsReadyBowl"));
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(ReadyTag))
		{
			return;
		}
	}

	DestroyByTag(World, GymLightTag);

	const FLinearColor KeyColor = bOldSchoolGym ? FLinearColor(1.f, 0.82f, 0.62f) : FLinearColor(1.f, 0.96f, 0.90f);
	const FLinearColor FillColor = bOldSchoolGym ? FLinearColor(1.f, 0.74f, 0.52f) : FLinearColor(0.85f, 0.90f, 1.f);
	const float KeyIntensity = bOldSchoolGym ? 6.5f : 10.f;
	const float FillIntensity = bOldSchoolGym ? 2.8f : 6.f;

	// Directional, spot, and point lights default to Stationary. This project
	// disables static lighting, so a shadow-casting stationary light has no
	// built channel and the ring stays black. The root has to be Movable
	// before the rotation, or PIE refuses the turn.
	auto ForceMovable = [](AActor* Actor, USceneComponent* Comp)
	{
		if (Actor)
		{
			if (USceneComponent* Root = Actor->GetRootComponent())
			{
				Root->SetMobility(EComponentMobility::Movable);
			}
		}
		if (Comp)
		{
			Comp->SetMobility(EComponentMobility::Movable);
		}
	};
	auto TuneKey = [KeyColor, KeyIntensity](UDirectionalLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetIntensity(KeyIntensity);
		Comp->SetLightColor(KeyColor);
		Comp->SetAtmosphereSunLight(false);
		Comp->SetCastShadows(true);
	};
	auto TuneFill = [FillColor, FillIntensity](UDirectionalLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetIntensity(FillIntensity);
		Comp->SetLightColor(FillColor);
		Comp->SetAtmosphereSunLight(false);
		Comp->SetCastShadows(false);
	};

	const FName KeyTag(TEXT("FB_P1KeyLight"));
	const FName FillTag(TEXT("FB_P1FillLight"));
	const FName DeadLightTag(TEXT("FB_P1DeadLight"));
	bool bHasKey = false;
	bool bHasFill = false;
	ADirectionalLight* KeyActor = nullptr;
	UDirectionalLightComponent* KeyComp = nullptr;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(DeadLightTag))
		{
			continue;
		}
		UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(It->GetLightComponent());
		ForceMovable(*It, Comp);
		if (It->ActorHasTag(FillTag))
		{
			bHasFill = true;
			TuneFill(Comp);
		}
		else
		{
			bHasKey = true;
			It->Tags.Add(KeyTag);
			TuneKey(Comp);
			It->SetActorRotation(FRotator(-48.f, 30.f, 0.f));
			KeyActor = *It;
			KeyComp = Comp;
		}
	}
	if (KeyComp && KeyComp->Mobility != EComponentMobility::Movable)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell: %s stayed mobility %d with static lighting off. Replacing it with a movable key."),
			KeyActor ? *KeyActor->GetName() : TEXT("key"), (int32)KeyComp->Mobility);
		KeyComp->SetIntensity(0.f);
		KeyComp->SetVisibility(false);
		if (KeyActor)
		{
			KeyActor->Tags.Add(DeadLightTag);
			KeyActor->SetActorHiddenInGame(true);
		}
		bHasKey = false;
		KeyComp = nullptr;
		KeyActor = nullptr;
	}
	if (!bHasKey)
	{
		FActorSpawnParameters Params;
		ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 900.f), FRotator(-48.f, 30.f, 0.f), Params);
		if (Light)
		{
			Light->Tags.Add(ArenaTag);
			Light->Tags.Add(KeyTag);
			UDirectionalLightComponent* SpawnedKey = Cast<UDirectionalLightComponent>(Light->GetLightComponent());
			ForceMovable(Light, SpawnedKey);
			TuneKey(SpawnedKey);
			KeyActor = Light;
			KeyComp = SpawnedKey;
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
			UDirectionalLightComponent* SpawnedFill = Cast<UDirectionalLightComponent>(Fill->GetLightComponent());
			ForceMovable(Fill, SpawnedFill);
			TuneFill(SpawnedFill);
		}
	}

	auto TuneSky = [bOldSchoolGym](USkyLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetIntensity(bOldSchoolGym ? 0.55f : 2.2f);
		Comp->bLowerHemisphereIsBlack = false;
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->RecaptureSky();
	};

	bool bHasSky = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		bHasSky = true;
		USkyLightComponent* SkyComp = It->GetLightComponent();
		ForceMovable(*It, SkyComp);
		TuneSky(SkyComp);
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

	auto TuneSpot = [bOldSchoolGym](USpotLightComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetIntensity(bOldSchoolGym ? 7000.f : 18000.f);
		Comp->SetLightColor(bOldSchoolGym ? FLinearColor(1.f, 0.78f, 0.55f) : FLinearColor(1.f, 0.97f, 0.92f));
		Comp->SetAttenuationRadius(2200.f);
		Comp->SetInnerConeAngle(28.f);
		Comp->SetOuterConeAngle(70.f);
		Comp->SetCastShadows(false);
	};

	bool bHasSpots = false;
	for (TActorIterator<ASpotLight> It(World); It; ++It)
	{
		USpotLightComponent* SpotComp = Cast<USpotLightComponent>(It->GetLightComponent());
		ForceMovable(*It, SpotComp);
		if (!It->ActorHasTag(ArenaTag))
		{
			continue;
		}
		bHasSpots = true;
		TuneSpot(SpotComp);
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
		ForceMovable(*It, It->GetLightComponent());
		if (It->ActorHasTag(ArenaTag))
		{
			bHasPoint = true;
			if (UPointLightComponent* Comp = Cast<UPointLightComponent>(It->GetLightComponent()))
			{
				Comp->SetMobility(EComponentMobility::Movable);
				Comp->SetIntensity(bOldSchoolGym ? 2500.f : 6000.f);
				Comp->SetAttenuationRadius(1800.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(bOldSchoolGym ? FLinearColor(1.f, 0.72f, 0.45f) : FLinearColor(1.f, 0.95f, 0.88f));
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
				Comp->SetMobility(EComponentMobility::Movable);
				Comp->SetIntensity(bOldSchoolGym ? 2500.f : 6000.f);
				Comp->SetAttenuationRadius(1800.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(bOldSchoolGym ? FLinearColor(1.f, 0.72f, 0.45f) : FLinearColor(1.f, 0.95f, 0.88f));
			}
		}
	}

	if (bOldSchoolGym)
	{
		const FVector BulbLocs[] = {
			FVector(-420.f, 420.f, 470.f),
			FVector(420.f, 420.f, 470.f),
			FVector(-420.f, -280.f, 470.f),
			FVector(420.f, -280.f, 470.f),
			FVector(0.f, 1650.f, 320.f)
		};
		for (const FVector& Loc : BulbLocs)
		{
			FActorSpawnParameters Params;
			APointLight* Bulb = World->SpawnActor<APointLight>(Loc, FRotator::ZeroRotator, Params);
			if (!Bulb)
			{
				continue;
			}
			Bulb->Tags.Add(ArenaTag);
			Bulb->Tags.Add(GymLightTag);
			if (UPointLightComponent* Comp = Cast<UPointLightComponent>(Bulb->GetLightComponent()))
			{
				ForceMovable(Bulb, Comp);
				Comp->SetMobility(EComponentMobility::Movable);
				Comp->SetIntensity(4500.f);
				Comp->SetAttenuationRadius(900.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.62f, 0.32f));
			}
		}
	}

	if (KeyActor)
	{
		KeyActor->Tags.Remove(FName(TEXT("FB_P1LightsReadyGym")));
		KeyActor->Tags.Remove(FName(TEXT("FB_P1LightsReadyBowl")));
		KeyActor->Tags.Add(ReadyTag);
	}

	const int32 KeyMobility = KeyComp ? (int32)KeyComp->Mobility : -1;
	UE_LOG(LogTemp, Warning, TEXT("FinalBell SpawnLighting gym=%d key=%s mobility=%d"),
		bOldSchoolGym ? 1 : 0,
		KeyActor ? *KeyActor->GetName() : TEXT("none"),
		KeyMobility);
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
		// A player pawn must not be reused as the opponent. That is the case
		// that left the ring with one fighter and refused the bout.
		if (It->ActorHasTag(OpponentTag) && !It->IsPlayerControlled())
		{
			It->ApplyBasicSkin(EFBCornerSkin::RedCorner);
			if (!It->GetController())
			{
				It->SpawnDefaultController();
			}
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
		Opponent->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
		if (!Opponent->GetController())
		{
			Opponent->SpawnDefaultController();
		}
	}
	return Opponent;
}

AFBBroadcastCamera* UFBP1ArenaBootstrap::SpawnCamera(UWorld* World, AFBBoxerCharacter* A, AFBBoxerCharacter* B)
{
	AFBBroadcastCamera* Cam = nullptr;
	for (TActorIterator<AFBBroadcastCamera> It(World); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		const AActor* Owner = It->GetOwner();
		if (!Owner || Owner == A)
		{
			Cam = *It;
			if (!Owner)
			{
				break;
			}
		}
	}

	if (!Cam)
	{
		Cam = World->SpawnActor<AFBBroadcastCamera>(FVector(0.f, -280.f, 170.f), FRotator(-12.f, 90.f, 0.f));
		if (Cam)
		{
			Cam->Tags.Add(ArenaTag);
		}
	}
	if (!Cam)
	{
		return nullptr;
	}

	Cam->SetFighters(A, B);
	if (A)
	{
		if (APlayerController* PC = Cast<APlayerController>(A->GetController()))
		{
			Cam->ForceViewForController(PC);
		}
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController() && PC->GetPawn() == A)
		{
			Cam->ForceViewForController(PC);
		}
	}
	return Cam;
}

void UFBP1ArenaBootstrap::EnsureArena(UWorld* World, AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent, AFBBroadcastCamera*& OutCamera, bool bSpawnOpponentIfMissing, bool bOldSchoolGym)
{
	OutPlayer = nullptr;
	OutOpponent = nullptr;
	OutCamera = nullptr;
	if (!World)
	{
		return;
	}

	SpawnFloor(World, bOldSchoolGym);
	SpawnLighting(World, bOldSchoolGym);
	AFBRingBounds* Ring = SpawnRing(World);

	auto PlaceCorner = [Ring](AFBBoxerCharacter* Boxer, EFBCornerSkin Skin)
	{
		if (!Boxer)
		{
			return;
		}
		Boxer->ApplyBasicSkin(Skin);
		const UCapsuleComponent* Capsule = Boxer->GetCapsuleComponent();
		const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 37.f;
		FVector Location = FVector::ZeroVector;
		FRotator Facing = FRotator::ZeroRotator;
		if (!Ring || !Ring->GetCornerAnchor(Skin, Radius, Location, Facing))
		{
			return;
		}
		Location.Z = CanvasTopZ + 6.f + Boxer->GetStandingHalfHeight();
		Boxer->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Boxer->SetActorRotation(Facing);
	};

	OutPlayer = FindFirstPlayerBoxer(World);
	PlaceCorner(OutPlayer, EFBCornerSkin::BlueCorner);

	if (bSpawnOpponentIfMissing)
	{
		FVector RedLocation(215.f, -215.f, CanvasTopZ + 120.f);
		FRotator RedFacing = FRotator(0.f, -135.f, 0.f);
		const float Radius = (OutPlayer && OutPlayer->GetCapsuleComponent())
			? OutPlayer->GetCapsuleComponent()->GetScaledCapsuleRadius()
			: 37.f;
		if (Ring)
		{
			Ring->GetCornerAnchor(EFBCornerSkin::RedCorner, Radius, RedLocation, RedFacing);
			RedLocation.Z = CanvasTopZ + 6.f + (OutPlayer ? OutPlayer->GetStandingHalfHeight() : 112.f);
		}
		OutOpponent = SpawnOpponent(World, RedLocation);
		PlaceCorner(OutOpponent, EFBCornerSkin::RedCorner);
	}

	if (bSpawnOpponentIfMissing)
	{
		OutCamera = SpawnCamera(World, OutPlayer, OutOpponent);
	}

	UE_LOG(LogTemp, Log, TEXT("FinalBell P1 arena ready (player=%s opponent=%s spawnOpp=%d gym=%d)"),
		OutPlayer ? *OutPlayer->GetName() : TEXT("none"),
		OutOpponent ? *OutOpponent->GetName() : TEXT("none"),
		bSpawnOpponentIfMissing ? 1 : 0,
		bOldSchoolGym ? 1 : 0);
}

void UFBP1ArenaBootstrap::EnsureMatchLook(UWorld* World, bool bOldSchoolGym, AFBBoxerCharacter* Player, AFBBoxerCharacter* Opponent)
{
	if (!World)
	{
		return;
	}
	SpawnFloor(World, bOldSchoolGym);
	SpawnLighting(World, bOldSchoolGym);
	SpawnRing(World);
	AFBBroadcastCamera* Cam = SpawnCamera(World, Player, Opponent);
	if (!Cam)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}
		if (PC->GetPawn() && PC->GetPawn() != Player)
		{
			continue;
		}
		Cam->ForceViewForController(PC);
	}
}

namespace BagRoom
{
	const FName ShellTag(TEXT("FB_BagRoom"));
	const FName LightTag(TEXT("FB_BagRoomLight"));

	// The gym floor and the wood hall both top out at Z=-90. This room uses that plane.
	// The hall runs Y=1400 to 1900 and is 280 cm wide. The gym north-wall gap matches it.
	constexpr float FloorTopZ = -90.f;
	constexpr float FloorThickness = 16.f;
	constexpr float SouthInnerY = 1900.f;
	constexpr float InteriorDepth = 780.f;
	constexpr float NorthInnerY = SouthInnerY + InteriorDepth;
	constexpr float InnerHalfX = 560.f;
	constexpr float WallThickness = 40.f;
	constexpr float CeilingClear = 320.f;
	constexpr float DoorHalfWidth = 140.f;

	// Leather center sits this far south of the north inner face, so a swing misses the brick.
	constexpr float BagInset = 155.f;
	constexpr float BagY = NorthInnerY - BagInset;
	// 86 cm of air under the 132 cm bag, then the 34 cm chain, up from the walk surface.
	constexpr float HangZ = FloorTopZ + 86.f + 132.f + 34.f;
	constexpr float RowSpacing = 200.f;
	constexpr float StandOff = 175.f;

	constexpr float CameraSide = 300.f;
	constexpr float CameraBack = 290.f;
	constexpr float CameraHeight = 150.f;
	constexpr float CameraLook = 115.f;

	constexpr float CeilingBottomZ = FloorTopZ + CeilingClear;
	constexpr float WallBottomZ = FloorTopZ - 8.f;
	constexpr float WallHeight = CeilingBottomZ - WallBottomZ;
	constexpr float WallCenterZ = (WallBottomZ + CeilingBottomZ) * 0.5f;
	constexpr float WallHalf = WallThickness * 0.5f;

	FVector ScaleCm(float SizeXcm, float SizeYcm, float SizeZcm)
	{
		return FVector(SizeXcm / 100.f, SizeYcm / 100.f, SizeZcm / 100.f);
	}

	AStaticMeshActor* Piece(
		UWorld* World,
		UStaticMesh* Mesh,
		const FVector& Loc,
		const FVector& Scale,
		const FLinearColor& Color,
		bool bCollision)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->Tags.Add(ShellTag);
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
}

void UFBP1ArenaBootstrap::DestroyBagRoom(UWorld* World)
{
	if (!World)
	{
		return;
	}
	TArray<AActor*> ToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(BagRoom::ShellTag) || It->ActorHasTag(BagRoom::LightTag))
		{
			ToDestroy.Add(*It);
		}
	}
	for (AActor* Actor : ToDestroy)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

FFBBagRoomLayout UFBP1ArenaBootstrap::GetBagRoomLayout()
{
	FFBBagRoomLayout Layout;
	Layout.FloorTopZ = BagRoom::FloorTopZ;
	Layout.SouthInnerY = BagRoom::SouthInnerY;
	Layout.NorthInnerY = BagRoom::NorthInnerY;
	Layout.InnerHalfX = BagRoom::InnerHalfX;
	Layout.BagY = BagRoom::BagY;
	Layout.HangZ = BagRoom::HangZ;
	Layout.RowSpacing = BagRoom::RowSpacing;
	Layout.StandOff = BagRoom::StandOff;
	Layout.CameraSide = BagRoom::CameraSide;
	Layout.CameraBack = BagRoom::CameraBack;
	Layout.CameraHeight = BagRoom::CameraHeight;
	Layout.CameraLook = BagRoom::CameraLook;
	return Layout;
}

void UFBP1ArenaBootstrap::EnsureBagRoom(UWorld* World, AFBHeavyBag*& OutActiveBag)
{
	OutActiveBag = nullptr;
	if (!World)
	{
		return;
	}

	// The key light has to be Movable. r.AllowStaticLighting is off, so a
	// stationary directional leaves this room black the same way the ring went black.
	SpawnLighting(World, true);

	bool bHasShell = false;
	bool bHasRoomLight = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(BagRoom::ShellTag))
		{
			bHasShell = true;
		}
		if (It->ActorHasTag(BagRoom::LightTag))
		{
			bHasRoomLight = true;
		}
		if (AFBHeavyBag* Bag = Cast<AFBHeavyBag>(*It))
		{
			if (Bag->IsActiveBag())
			{
				OutActiveBag = Bag;
			}
		}
	}

	const float OuterX = BagRoom::InnerHalfX + BagRoom::WallThickness;
	const float SouthOuterY = BagRoom::SouthInnerY - BagRoom::WallThickness;
	const float NorthOuterY = BagRoom::NorthInnerY + BagRoom::WallThickness;
	const float FloorCenterZ = BagRoom::FloorTopZ - BagRoom::FloorThickness * 0.5f;
	const float CeilingCenterZ = BagRoom::CeilingBottomZ + 6.f;
	const FVector RoomCenter(0.f, (BagRoom::SouthInnerY + BagRoom::NorthInnerY) * 0.5f, 0.f);

	if (!bHasShell)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (Cube)
		{
			const FLinearColor Wood(0.38f, 0.23f, 0.13f);
			const FLinearColor WoodDark(0.22f, 0.13f, 0.08f);
			const FLinearColor Brick(0.45f, 0.22f, 0.15f);
			const FLinearColor BrickDark(0.30f, 0.14f, 0.11f);
			const FLinearColor Ceiling(0.14f, 0.12f, 0.10f);
			const FLinearColor Glass(0.62f, 0.72f, 0.68f);

			// Floor top matches the gym and the hall. Walls sit on it. The ceiling sits on the walls.
			const float FloorSizeX = OuterX * 2.f;
			const float FloorSizeY = NorthOuterY - SouthOuterY;
			BagRoom::Piece(
				World, Cube,
				FVector(0.f, (SouthOuterY + NorthOuterY) * 0.5f, FloorCenterZ),
				BagRoom::ScaleCm(FloorSizeX, FloorSizeY, BagRoom::FloorThickness),
				Wood, true);
			for (int32 Board = 0; Board < 5; ++Board)
			{
				const float BoardY = BagRoom::SouthInnerY + 80.f + Board * 140.f;
				BagRoom::Piece(
					World, Cube,
					FVector(0.f, BoardY, BagRoom::FloorTopZ + 1.f),
					BagRoom::ScaleCm((BagRoom::InnerHalfX - 40.f) * 2.f, 6.f, 2.f),
					WoodDark, false);
			}

			const FVector WallScaleNS = BagRoom::ScaleCm(FloorSizeX, BagRoom::WallThickness, BagRoom::WallHeight);
			const FVector WallScaleEW = BagRoom::ScaleCm(BagRoom::WallThickness, FloorSizeY, BagRoom::WallHeight);
			BagRoom::Piece(World, Cube, FVector(0.f, NorthOuterY - BagRoom::WallHalf, BagRoom::WallCenterZ), WallScaleNS, Brick, true);

			const float DoorGap = BagRoom::DoorHalfWidth * 2.f;
			const float SouthSegment = (FloorSizeX - DoorGap) * 0.5f;
			const float SouthCenterX = BagRoom::DoorHalfWidth + SouthSegment * 0.5f;
			const FVector SouthScale = BagRoom::ScaleCm(SouthSegment, BagRoom::WallThickness, BagRoom::WallHeight);
			const float SouthCenterY = BagRoom::SouthInnerY - BagRoom::WallHalf;
			BagRoom::Piece(World, Cube, FVector(-SouthCenterX, SouthCenterY, BagRoom::WallCenterZ), SouthScale, BrickDark, true);
			BagRoom::Piece(World, Cube, FVector(SouthCenterX, SouthCenterY, BagRoom::WallCenterZ), SouthScale, BrickDark, true);
			BagRoom::Piece(World, Cube, FVector(OuterX - BagRoom::WallHalf, RoomCenter.Y, BagRoom::WallCenterZ), WallScaleEW, Brick, true);
			BagRoom::Piece(World, Cube, FVector(-(OuterX - BagRoom::WallHalf), RoomCenter.Y, BagRoom::WallCenterZ), WallScaleEW, BrickDark, true);

			BagRoom::Piece(
				World, Cube,
				FVector(0.f, RoomCenter.Y, CeilingCenterZ),
				BagRoom::ScaleCm(FloorSizeX, FloorSizeY, 12.f),
				Ceiling, false);

			// Beam spans into both side walls. Its underside is the hang point.
			const float BeamHeight = 22.f;
			BagRoom::Piece(
				World, Cube,
				FVector(0.f, BagRoom::BagY, BagRoom::HangZ + BeamHeight * 0.5f),
				BagRoom::ScaleCm(FloorSizeX, 28.f, BeamHeight),
				WoodDark, false);

			// Short enclosed hall from the gym's north opening to this doorway. Training only.
			const float HallSouthY = 1410.f;
			const float HallNorthY = SouthOuterY + 20.f;
			const float HallCenterY = (HallSouthY + HallNorthY) * 0.5f;
			const float HallLength = HallNorthY - HallSouthY;
			const float HallOuterX = BagRoom::DoorHalfWidth + BagRoom::WallThickness;
			BagRoom::Piece(
				World, Cube,
				FVector(-BagRoom::DoorHalfWidth - BagRoom::WallHalf, HallCenterY, BagRoom::WallCenterZ),
				BagRoom::ScaleCm(BagRoom::WallThickness, HallLength, BagRoom::WallHeight),
				BrickDark, true);
			BagRoom::Piece(
				World, Cube,
				FVector(BagRoom::DoorHalfWidth + BagRoom::WallHalf, HallCenterY, BagRoom::WallCenterZ),
				BagRoom::ScaleCm(BagRoom::WallThickness, HallLength, BagRoom::WallHeight),
				Brick, true);
			BagRoom::Piece(
				World, Cube,
				FVector(-(BagRoom::DoorHalfWidth + BagRoom::WallHalf), 1650.f, FloorCenterZ),
				BagRoom::ScaleCm(BagRoom::WallThickness, 500.f, BagRoom::FloorThickness),
				Wood, true);
			BagRoom::Piece(
				World, Cube,
				FVector(BagRoom::DoorHalfWidth + BagRoom::WallHalf, 1650.f, FloorCenterZ),
				BagRoom::ScaleCm(BagRoom::WallThickness, 500.f, BagRoom::FloorThickness),
				Wood, true);
			BagRoom::Piece(
				World, Cube,
				FVector(0.f, HallCenterY, CeilingCenterZ),
				BagRoom::ScaleCm(HallOuterX * 2.f, HallLength, 12.f),
				Ceiling, false);

			BagRoom::Piece(
				World, Cube,
				FVector(0.f, NorthOuterY - BagRoom::WallHalf, BagRoom::FloorTopZ + 160.f),
				BagRoom::ScaleCm(320.f, 8.f, 90.f),
				Glass, false);
			BagRoom::Piece(
				World, Cube,
				FVector(OuterX - BagRoom::WallHalf, BagRoom::SouthInnerY + 280.f, BagRoom::FloorTopZ + 160.f),
				BagRoom::ScaleCm(8.f, 160.f, 80.f),
				Glass, false);
		}
	}

	if (!bHasRoomLight)
	{
		auto ForceMovable = [](AActor* Actor, USceneComponent* Comp)
		{
			if (Actor)
			{
				if (USceneComponent* Root = Actor->GetRootComponent())
				{
					Root->SetMobility(EComponentMobility::Movable);
				}
			}
			if (Comp)
			{
				Comp->SetMobility(EComponentMobility::Movable);
			}
		};

		const float BulbZ = BagRoom::CeilingBottomZ - 22.f;
		const FVector Bulbs[] = {
			FVector(-BagRoom::RowSpacing * 1.5f, BagRoom::BagY - 70.f, BulbZ),
			FVector(0.f, BagRoom::BagY - 70.f, BulbZ),
			FVector(BagRoom::RowSpacing * 1.5f, BagRoom::BagY - 70.f, BulbZ),
			FVector(0.f, 1650.f, BagRoom::FloorTopZ + 250.f)
		};
		for (const FVector& Loc : Bulbs)
		{
			FActorSpawnParameters Params;
			APointLight* Bulb = World->SpawnActor<APointLight>(Loc, FRotator::ZeroRotator, Params);
			if (!Bulb)
			{
				continue;
			}
			Bulb->Tags.Add(BagRoom::LightTag);
			if (UPointLightComponent* Comp = Cast<UPointLightComponent>(Bulb->GetLightComponent()))
			{
				ForceMovable(Bulb, Comp);
				Comp->SetIntensity(7500.f);
				Comp->SetAttenuationRadius(700.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.74f, 0.50f));
			}
		}

		const FVector Focus(0.f, BagRoom::BagY - BagRoom::StandOff * 0.5f, BagRoom::FloorTopZ + BagRoom::CameraLook);
		const FVector SpotLoc(-340.f, BagRoom::SouthInnerY + 180.f, BagRoom::FloorTopZ + 210.f);
		const FRotator Aim = (Focus - SpotLoc).Rotation();
		FActorSpawnParameters SpotParams;
		ASpotLight* Spot = World->SpawnActor<ASpotLight>(SpotLoc, Aim, SpotParams);
		if (Spot)
		{
			Spot->Tags.Add(BagRoom::LightTag);
			if (USpotLightComponent* Comp = Cast<USpotLightComponent>(Spot->GetLightComponent()))
			{
				ForceMovable(Spot, Comp);
				Comp->SetIntensity(14000.f);
				Comp->SetAttenuationRadius(1100.f);
				Comp->SetInnerConeAngle(18.f);
				Comp->SetOuterConeAngle(46.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.84f, 0.66f));
			}
		}

		const FVector FillLoc(280.f, BagRoom::SouthInnerY + 240.f, BagRoom::FloorTopZ + 200.f);
		const FRotator FillAim = (FVector(0.f, BagRoom::BagY, BagRoom::FloorTopZ + 150.f) - FillLoc).Rotation();
		ASpotLight* Fill = World->SpawnActor<ASpotLight>(FillLoc, FillAim, SpotParams);
		if (Fill)
		{
			Fill->Tags.Add(BagRoom::LightTag);
			if (USpotLightComponent* Comp = Cast<USpotLightComponent>(Fill->GetLightComponent()))
			{
				ForceMovable(Fill, Comp);
				Comp->SetIntensity(7000.f);
				Comp->SetAttenuationRadius(1000.f);
				Comp->SetInnerConeAngle(22.f);
				Comp->SetOuterConeAngle(58.f);
				Comp->SetCastShadows(false);
				Comp->SetLightColor(FLinearColor(1.f, 0.70f, 0.48f));
			}
		}
	}

	if (!World->GetAuthGameMode() || OutActiveBag)
	{
		return;
	}

	const float Slots[] = { -BagRoom::RowSpacing * 2.f, -BagRoom::RowSpacing, 0.f, BagRoom::RowSpacing, BagRoom::RowSpacing * 2.f };
	for (float SlotX : Slots)
	{
		const bool bActive = FMath::IsNearlyZero(SlotX);
		const FVector Hang(SlotX, BagRoom::BagY, BagRoom::HangZ);
		const FTransform Xform(FRotator::ZeroRotator, Hang);
		AFBHeavyBag* Bag = World->SpawnActorDeferred<AFBHeavyBag>(
			AFBHeavyBag::StaticClass(),
			Xform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Bag)
		{
			continue;
		}
		Bag->SetBagRole(bActive);
		Bag = Cast<AFBHeavyBag>(UGameplayStatics::FinishSpawningActor(Bag, Xform));
		if (bActive)
		{
			OutActiveBag = Bag;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("FinalBell bag room floorZ=%.0f bagY=%.0f hangZ=%.0f spacing=%.0f active=%s"),
		BagRoom::FloorTopZ,
		BagRoom::BagY,
		BagRoom::HangZ,
		BagRoom::RowSpacing,
		OutActiveBag ? *OutActiveBag->GetName() : TEXT("none"));
}
