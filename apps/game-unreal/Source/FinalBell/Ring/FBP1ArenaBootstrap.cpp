#include "Ring/FBP1ArenaBootstrap.h"
#include "Ring/FBRingBounds.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName ArenaTag(TEXT("FB_P1Arena"));
	const FName FloorTag(TEXT("FB_P1Floor"));
	const FName OpponentTag(TEXT("FB_P1Opponent"));
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

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Floor)
	{
		return nullptr;
	}

	Floor->SetActorScale3D(FVector(12.f, 12.f, 1.f));
	Floor->Tags.Add(FloorTag);
	Floor->Tags.Add(ArenaTag);
	if (UStaticMeshComponent* MeshComp = Floor->GetStaticMeshComponent())
	{
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetStaticMesh(PlaneMesh);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	}
	return Floor;
}

void UFBP1ArenaBootstrap::SpawnLighting(UWorld* World)
{
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(ArenaTag))
		{
			return;
		}
	}

	FActorSpawnParameters Params;
	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 500.f), FRotator(-40.f, 30.f, 0.f), Params);
	if (Light)
	{
		Light->Tags.Add(ArenaTag);
		if (UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
		{
			Comp->SetIntensity(8.f);
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
	}
	return Opponent;
}

AFBBroadcastCamera* UFBP1ArenaBootstrap::SpawnCamera(UWorld* World, AFBBoxerCharacter* A, AFBBoxerCharacter* B)
{
	for (TActorIterator<AFBBroadcastCamera> It(World); It; ++It)
	{
		It->SetFighters(A, B);
		return *It;
	}

	AFBBroadcastCamera* Cam = World->SpawnActor<AFBBroadcastCamera>(FVector(0.f, -600.f, 280.f), FRotator(-10.f, 90.f, 0.f));
	if (Cam)
	{
		Cam->Tags.Add(ArenaTag);
		Cam->SetFighters(A, B);
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
		OutPlayer->SetActorLocation(FVector(-220.f, 0.f, 120.f));
		OutPlayer->SetActorRotation(FRotator(0.f, 0.f, 0.f));
	}

	OutOpponent = SpawnOpponent(World, FVector(220.f, 0.f, 120.f));
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
	UE_LOG(LogTemp, Log, TEXT("FinalBell P1 arena ready (player=%s opponent=%s camera=%s)"),
		OutPlayer ? *OutPlayer->GetName() : TEXT("none"),
		OutOpponent ? *OutOpponent->GetName() : TEXT("none"),
		OutCamera ? *OutCamera->GetName() : TEXT("none"));
}
