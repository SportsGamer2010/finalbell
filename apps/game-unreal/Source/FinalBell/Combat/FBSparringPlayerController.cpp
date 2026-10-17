#include "Combat/FBSparringPlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameInputDeveloperSettings.h"

AFBSparringPlayerController::AFBSparringPlayerController()
{
	bReplicates = true;
	bEnableClickEvents = false;
	bShowMouseCursor = false;
}

void AFBSparringPlayerController::BeginPlay()
{
	Super::BeginPlay();

	TArray<FInputDeviceId> Devices;
	const int32 Num = IPlatformInputDeviceMapper::Get().GetAllConnectedInputDevices(Devices);
	UE_LOG(LogTemp, Warning, TEXT("FinalBell: %d connected input device(s)."), Num);
	for (const FInputDeviceId& Id : Devices)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell connected device id=%d"), Id.GetId());
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
		UE_LOG(LogTemp, Warning, TEXT("FinalBell: Still only KBM. Plug DualSense via USB, fully restart the editor after GameInput ini changes, then Play again."));
	}
}
