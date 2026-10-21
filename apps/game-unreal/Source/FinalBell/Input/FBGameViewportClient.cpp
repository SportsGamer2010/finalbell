#include "Input/FBGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Widgets/SViewport.h"

namespace
{
	int32 CountPieViewports()
	{
		if (!GEngine)
		{
			return 0;
		}
		int32 Count = 0;
		for (const FWorldContext& WC : GEngine->GetWorldContexts())
		{
			if (WC.WorldType == EWorldType::PIE && WC.GameViewport)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 ResolvePadUser(const FInputKeyEventArgs& EventArgs)
	{
		int32 PadUser = EventArgs.ControllerId;
		const FPlatformUserId UserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(EventArgs.InputDevice);
		if (UserId.IsValid())
		{
			PadUser = UserId.GetInternalId();
		}
		return PadUser;
	}
}

ULocalPlayer* UFBGameViewportClient::SetupInitialLocalPlayer(FString& OutError)
{
	ULocalPlayer* LocalPlayer = Super::SetupInitialLocalPlayer(OutError);
	BindPieLocalPlayerToGamepad();
	return LocalPlayer;
}

void UFBGameViewportClient::SetSelectPadMode(bool bOneHuman, bool bTwoHumans)
{
	bOneHumanPads = bOneHuman;
	bTwoHumanPads = bTwoHumans;
	if (bTwoHumans)
	{
		SetForceDisableSplitscreen(true);
	}
}

void UFBGameViewportClient::ClaimPadsForLocalPlayer()
{
	if (bOneHumanPads || CountPieViewports() >= 2)
	{
		return;
	}
	if (bTwoHumanPads)
	{
		UGameInstance* ActiveInstance = GetGameInstance();
		if (!ActiveInstance || ActiveInstance->GetNumLocalPlayers() < 1)
		{
			return;
		}
		ULocalPlayer* First = ActiveInstance->GetLocalPlayerByIndex(0);
		ULocalPlayer* Second = ActiveInstance->GetNumLocalPlayers() > 1 ? ActiveInstance->GetLocalPlayerByIndex(1) : nullptr;
		if (!First)
		{
			return;
		}
		IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
		const FInputDeviceId Keyboard = Mapper.GetDefaultInputDevice();
		TArray<FInputDeviceId> Devices;
		Mapper.GetAllConnectedInputDevices(Devices);
		int32 Gamepad = 0;
		for (const FInputDeviceId& Device : Devices)
		{
			if (!Device.IsValid() || Device == Keyboard)
			{
				continue;
			}
			ULocalPlayer* OwnerPlayer = Gamepad == 0 ? First : (Gamepad == 1 ? Second : nullptr);
			++Gamepad;
			if (!OwnerPlayer)
			{
				continue;
			}
			const FPlatformUserId Want = OwnerPlayer->GetPlatformUserId();
			if (!Want.IsValid())
			{
				continue;
			}
			const FPlatformUserId Owner = Mapper.GetUserForInputDevice(Device);
			if (Owner == Want)
			{
				continue;
			}
			if (Owner.IsValid())
			{
				Mapper.Internal_ChangeInputDeviceUserMapping(Device, Want, Owner);
			}
			else
			{
				Mapper.Internal_MapInputDeviceToUser(Device, Want, EInputDeviceConnectionState::Connected);
			}
		}
		return;
	}
	ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (!LocalPlayer)
	{
		return;
	}
	const FPlatformUserId Want = LocalPlayer->GetPlatformUserId();
	if (!Want.IsValid())
	{
		return;
	}

	IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
	const FInputDeviceId Keyboard = Mapper.GetDefaultInputDevice();
	TArray<FInputDeviceId> Devices;
	Mapper.GetAllConnectedInputDevices(Devices);
	for (const FInputDeviceId& Device : Devices)
	{
		if (!Device.IsValid() || Device == Keyboard)
		{
			continue;
		}
		const FPlatformUserId Owner = Mapper.GetUserForInputDevice(Device);
		if (Owner == Want)
		{
			continue;
		}
		if (Owner.IsValid())
		{
			Mapper.Internal_ChangeInputDeviceUserMapping(Device, Want, Owner);
		}
		else
		{
			Mapper.Internal_MapInputDeviceToUser(Device, Want, EInputDeviceConnectionState::Connected);
		}
	}
}

void UFBGameViewportClient::BindPieLocalPlayerToGamepad()
{
#if WITH_EDITOR
	if (!GEngine)
	{
		return;
	}

	const FWorldContext* Ctx = GEngine->GetWorldContextFromGameViewport(this);
	if (!Ctx || Ctx->WorldType != EWorldType::PIE || Ctx->PIEInstance < 0)
	{
		return;
	}

	// Solo PIE stays on the front-end local player. Only a second window takes the next id.
	if (Ctx->PIEInstance > 0)
	{
		if (ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
		{
			LocalPlayer->SetControllerId(Ctx->PIEInstance);
			bPieBindingApplied = true;
			UE_LOG(LogTemp, Warning,
				TEXT("FinalBell: PIE instance %d bound to controller/user %d"),
				Ctx->PIEInstance, Ctx->PIEInstance);
		}
	}
#endif
	ClaimPadsForLocalPlayer();
}

void UFBGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ClaimPadsForLocalPlayer();
}

void UFBGameViewportClient::RemapControllerInput(FInputKeyEventArgs& InOutEventArgs)
{
#if WITH_EDITOR
	if (InOutEventArgs.IsGamepad() && GEngine)
	{
		if (const FWorldContext* Ctx = GEngine->GetWorldContextFromGameViewport(this))
		{
			if (Ctx->WorldType == EWorldType::PIE && Ctx->PIEInstance >= 0)
			{
				const int32 PieCount = CountPieViewports();
				const int32 PadUser = ResolvePadUser(InOutEventArgs);

				// Solo PIE: keep this pad's device id, and give it to the one local player.
				if (PieCount <= 1)
				{
					if (ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
					{
						ClaimPadsForLocalPlayer();
						InOutEventArgs.ControllerId = LP->GetControllerId();
					}
					return;
				}

				// Multi PIE: pad N stays on PIE instance N.
				if (PadUser == Ctx->PIEInstance)
				{
					return;
				}
			}
		}
	}
#endif
	Super::RemapControllerInput(InOutEventArgs);
}

bool UFBGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
#if WITH_EDITOR
	if (EventArgs.IsGamepad() && RouteGamepadToMatchingPieViewport(EventArgs, false))
	{
		return true;
	}
#endif
	return Super::InputKey(EventArgs);
}

bool UFBGameViewportClient::InputAxis(const FInputKeyEventArgs& EventArgs)
{
#if WITH_EDITOR
	if (EventArgs.IsGamepad() && RouteGamepadToMatchingPieViewport(EventArgs, true))
	{
		return true;
	}
#endif
	return Super::InputAxis(EventArgs);
}

bool UFBGameViewportClient::RouteGamepadToMatchingPieViewport(const FInputKeyEventArgs& EventArgs, bool bAxis)
{
	if (!GEngine || !EventArgs.Viewport || !EventArgs.Viewport->IsPlayInEditorViewport())
	{
		return false;
	}

	UWorld* PieWorld = GetWorld();
	if (!PieWorld)
	{
		return false;
	}

	const FWorldContext* OwnCtx = GEngine->GetWorldContextFromWorld(PieWorld);
	if (!OwnCtx || OwnCtx->WorldType != EWorldType::PIE)
	{
		return false;
	}

	if (CountPieViewports() < 2)
	{
		return false;
	}

	const int32 DesiredPie = ResolvePadUser(EventArgs);
	if (DesiredPie == OwnCtx->PIEInstance)
	{
		return false;
	}

	for (const FWorldContext& WC : GEngine->GetWorldContexts())
	{
		if (WC.WorldType == EWorldType::PIE && WC.PIEInstance == DesiredPie && WC.GameViewport && WC.GameViewport != this)
		{
			if (bAxis)
			{
				WC.GameViewport->InputAxis(EventArgs);
			}
			else
			{
				WC.GameViewport->InputKey(EventArgs);
			}
			return true;
		}
	}

	return false;
}
