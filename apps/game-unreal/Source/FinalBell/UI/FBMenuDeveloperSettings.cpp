#include "UI/FBMenuDeveloperSettings.h"

UFBMenuDeveloperSettings::UFBMenuDeveloperSettings()
{
}

const UFBMenuDeveloperSettings* UFBMenuDeveloperSettings::Get()
{
	return GetDefault<UFBMenuDeveloperSettings>();
}
