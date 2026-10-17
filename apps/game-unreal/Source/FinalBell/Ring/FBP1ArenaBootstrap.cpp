#include "Ring/FBP1ArenaBootstrap.h"
#include "Ring/FBRingBounds.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
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
	const FName OpponentTag(TEXT("FB_P1Opponent"));

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

	AStaticMeshActor* SpawnMeshActor(UWorld* World, UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, const FName& Tag)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->Tags.Add(Tag);
		Actor->Tags.Add(ArenaTag);
		if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
		{
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetStaticMesh(Mesh);
			Comp->SetWorldScale3D(Scale);
			Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Comp->SetCollisionResponseToAllChannels(ECR_Block);
			TintMesh(Comp, Color);
		}
		return Actor;
	}
}

AActor* UFBP1ArenaBootstrap::SpawnFloor(UWorld* World)
{
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FloorTag))
		{
			return *It;
		}
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	// Thick canvas platform (visible ring floor)
	AStaticMeshActor* Floor = SpawnMeshActor(
		World,
		Cube,
		FVector(0.f, 0.f, -20.f),
		FVector(10.f, 10.f, 0.4f),
		FLinearColor(0.75f, 0.78f, 0.85f),
		FloorTag);

	// Corner posts (simple vertical cubes)
	const FLinearColor PostColor(0.9f, 0.85f, 0.2f);
	const float Half = 480.f;
	const TArray<FVector> Posts = {
		FVector(Half, Half, 60.f),
		FVector(Half, -Half, 60.f),
		FVector(-Half, Half, 60.f),
		FVector(-Half, -Half, 60.f)
	};
	for (const FVector& P : Posts)
	{
		SpawnMeshActor(World, Cube, P, FVector(0.25f, 0.25f, 1.4f), PostColor, ArenaTag);
	}

	return Floor;
}

void UFBP1ArenaBootstrap::SpawnLighting(UWorld* World)
{
	bool bHasDir = false;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(ArenaTag))
		{
			bHasDir = true;
			break;
		}
	}
	if (!bHasDir)
	{
		FActorSpawnParameters Params;
		ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 800.f), FRotator(-50.f, 35.f, 0.f), Params);
		if (Light)
		{
			Light->Tags.Add(ArenaTag);
			if (UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
			{
				Comp->SetIntensity(12.f);
				Comp->SetAtmosphereSunLight(true);
			}
		}
	}

	bool bHasSky = false;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		bHasSky = true;
		break;
	}
	if (!bHasSky)
	{
		ASkyLight* Sky = World->SpawnActor<ASkyLight>();
		if (Sky)
		{
			Sky->Tags.Add(ArenaTag);
			if (USkyLightComponent* Comp = Sky->GetLightComponent())
			{
				Comp->SetIntensity(1.2f);
				Comp->SetMobility(EComponentMobility::Movable);
				Comp->RecaptureSky();
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

	AFBRingBounds* Ring = World->SpawnActor<AFBRingBounds>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
	if (Ring)
	{
		Ring->Tags.Add(ArenaTag);
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
			It->ApplyPlaceholderAppearance(true);
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
		Opponent->ApplyPlaceholderAppearance(true);
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

	AFBBroadcastCamera* Cam = World->SpawnActor<AFBBroadcastCamera>(FVector(0.f, -900.f, 420.f), FRotator(-15.f, 90.f, 0.f));
	if (Cam)
	{
		Cam->Tags.Add(ArenaTag);
		Cam->SetFighters(A, B);
		Cam->ForcePossessedView();
	}
	return Cam;
}

void UFBP1ArenaBootstrap::EnsureArena(UWorld* World, AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent, AFBBroadcastCamera*& OutCamera)
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
	if (OutPlayer)
	{
		OutPlayer->SetActorLocation(FVector(-220.f, 0.f, 96.f));
		OutPlayer->SetActorRotation(FRotator(0.f, 0.f, 0.f));
		OutPlayer->ApplyPlaceholderAppearance(false);
	}

	OutOpponent = SpawnOpponent(World, FVector(220.f, 0.f, 96.f));
	if (OutPlayer && OutOpponent)
	{
		const FVector ToOpp = (OutOpponent->GetActorLocation() - OutPlayer->GetActorLocation()).GetSafeNormal2D();
		if (!ToOpp.IsNearlyZero())
		{
			OutPlayer->SetActorRotation(ToOpp.Rotation());
			OutOpponent->SetActorRotation((-ToOpp).Rotation());
		}
	}

	OutCamera = SpawnCamera(World, OutPlayer, OutOpponent);
	UE_LOG(LogTemp, Log, TEXT("FinalBell P1 arena ready (player=%s opponent=%s camera=%s) — look for BLUE vs RED placeholders on a gray platform."),
		OutPlayer ? *OutPlayer->GetName() : TEXT("none"),
		OutOpponent ? *OutOpponent->GetName() : TEXT("none"),
		OutCamera ? *OutCamera->GetName() : TEXT("none"));
}
