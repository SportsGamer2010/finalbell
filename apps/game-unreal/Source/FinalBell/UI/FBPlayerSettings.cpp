#include "UI/FBPlayerSettings.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

const TCHAR* UFBPlayerSettingsSave::SlotName = TEXT("FBPlayerSettings");

namespace
{
	TWeakObjectPtr<UFBPlayerSettingsSave> CachedSettings;
}

UFBPlayerSettingsSave* UFBPlayerSettingsSave::LoadOrCreate()
{
	if (UFBPlayerSettingsSave* Existing = CachedSettings.Get())
	{
		return Existing;
	}
	UFBPlayerSettingsSave* Settings = nullptr;
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
	{
		Settings = Cast<UFBPlayerSettingsSave>(Loaded);
	}
	if (!Settings)
	{
		Settings = Cast<UFBPlayerSettingsSave>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	}
	if (Settings)
	{
		Settings->AddToRoot();
		CachedSettings = Settings;
	}
	return Settings;
}

void UFBPlayerSettingsSave::WriteToDisk() const
{
	UGameplayStatics::SaveGameToSlot(const_cast<UFBPlayerSettingsSave*>(this), SlotName, 0);
}

void UFBPlayerSettingsSave::ApplyAudio(const UObject* WorldContext) const
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	if (FAudioDeviceHandle Device = World->GetAudioDevice())
	{
		Device->SetTransientPrimaryVolume(FMath::Clamp(MasterVolume, 0.f, 1.f));
	}
}
