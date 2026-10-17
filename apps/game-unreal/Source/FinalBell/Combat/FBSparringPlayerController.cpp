#include "Combat/FBSparringPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameInputDeveloperSettings.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Input/FBGameViewportClient.h"
#include "Widgets/SViewport.h"

AFBSparringPlayerController::AFBSparringPlayerController()
{
	bReplicates = true;
	bEnableClickEvents = false;
	bShowMouseCursor = false;
}

void AFBSparringPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (GEngine && GetWorld())
	{
		if (const FWorldContext* Ctx = GEngine->GetWorldContextFromWorld(GetWorld()))
		{
			if (Ctx->WorldType == EWorldType::PIE && Ctx->PIEInstance >= 0)
			{
				if (ULocalPlayer* LP = GetLocalPlayer())
				{
					LP->SetControllerId(Ctx->PIEInstance);
				}
				if (UFBGameViewportClient* VC = Cast<UFBGameViewportClient>(GetWorld()->GetGameViewport()))
				{
					VC->BindPieLocalPlayerToGamepad();
				}
				else if (UGameViewportClient* VCBase = GetWorld()->GetGameViewport())
				{
					if (TSharedPtr<SViewport> Widget = VCBase->GetGameViewportWidget())
					{
						if (FSlateApplication::IsInitialized())
						{
							FSlateApplication::Get().SetUserFocus(
								static_cast<uint32>(Ctx->PIEInstance),
								Widget,
								EFocusCause::SetDirectly);
						}
					}
				}
			}
		}
	}
#endif

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FinalBell: local player ControllerId=%d PlatformUser=%d"),
			LP->GetControllerId(),
			LP->GetPlatformUserId().GetInternalId());
	}

	TArray<FInputDeviceId> Devices;
	const int32 Num = IPlatformInputDeviceMapper::Get().GetAllConnectedInputDevices(Devices);
	UE_LOG(LogTemp, Warning, TEXT("FinalBell: %d connected input device(s)."), Num);
	for (const FInputDeviceId& Id : Devices)
	{
		const FPlatformUserId User = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(Id);
		UE_LOG(LogTemp, Warning, TEXT("FinalBell connected device id=%d user=%d"),
			Id.GetId(), User.GetInternalId());
	}

	if (const UGameInputPlatformSettings* GI = UGameInputPlatformSettings::Get())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FinalBell GameInput flags: Gamepad=%d Controller=%d Raw=%d SpecialRequireConfig=%d"),
			GI->bProcessGamepad ? 1 : 0,
			GI->bProcessController ? 1 : 0,
			GI->bProcessRawInput ? 1 : 0,
			GI->bSpecialDevicesRequireExplicitDeviceConfiguration ? 1 : 0);
	}

	if (Num <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell: Still only KBM. Plug pad via USB, fully restart the editor after GameInput ini changes, then Play again."));
	}
}
