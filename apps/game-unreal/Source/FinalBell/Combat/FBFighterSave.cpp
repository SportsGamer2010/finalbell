#include "Combat/FBFighterSave.h"
#include "Combat/FBBoxerCustomization.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* RosterSlot = TEXT("FBFighterRoster");
}

UFBFighterRosterSave* UFBFighterRosterSave::LoadOrCreate()
{
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(RosterSlot, 0))
	{
		if (UFBFighterRosterSave* Roster = Cast<UFBFighterRosterSave>(Loaded))
		{
			return Roster;
		}
	}
	return Cast<UFBFighterRosterSave>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
}

bool UFBFighterRosterSave::TrySoloCareerPair(FFBFighterRecord& OutPlayer, FFBFighterRecord& OutOpponent)
{
	UFBFighterRosterSave* Saved = LoadOrCreate();
	if (!Saved)
	{
		return false;
	}
	const FFBFighterRecord* Only = nullptr;
	int32 ReadyCount = 0;
	for (const FFBFighterRecord& Record : Saved->Created)
	{
		if (!Record.bPremade && FFBFighterBalance::IsReady(Record.Ratings))
		{
			Only = &Record;
			++ReadyCount;
		}
	}
	if (ReadyCount != 1 || !Only)
	{
		return false;
	}
	FFBFighterRecord Player = *Only;
	Player.WeightClass = FFBFighterBalance::ClassForDivision(Player.Division);
	Player.RulesetId = TEXT("CompetitiveFair");
	Player.RulesetVersion = FFBFighterBalance::RulesetVersion;
	TArray<FFBFighterRecord> Premades;
	FFBFighterBalance::GetPremades(Premades);
	for (const FFBFighterRecord& Premade : Premades)
	{
		if (Premade.bPremade && Premade.WeightClass == Player.WeightClass && FFBFighterBalance::IsReady(Premade.Ratings))
		{
			OutPlayer = Player;
			OutOpponent = Premade;
			return true;
		}
	}
	return false;
}

void UFBFighterRosterSave::WriteToDisk() const
{
	UGameplayStatics::SaveGameToSlot(const_cast<UFBFighterRosterSave*>(this), RosterSlot, 0);
}

bool UFBFighterRosterSave::TryAdd(FFBFighterRecord Record, FString& OutWhy)
{
	if (!CanAdd())
	{
		OutWhy = TEXT("Ten created fighters is the limit. Delete one to save another.");
		return false;
	}
	if (!FFBFighterBalance::IsDraftLegal(Record.Ratings))
	{
		OutWhy = TEXT("That build is outside 50-95 or costs more than 600.");
		return false;
	}
	if (Record.Id.IsEmpty())
	{
		Record.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	}
	Record.bPremade = false;
	Record.WeightClass = FFBFighterBalance::ClassForDivision(Record.Division);
	Record.RulesetId = TEXT("CompetitiveFair");
	Record.RulesetVersion = FFBFighterBalance::RulesetVersion;
	FFBBoxerCustomization::Sanitize(Record.Look);
	if (!FFBBoxerCustomization::Validate(Record.Look, Record.WeightClass, OutWhy))
	{
		return false;
	}
	Created.Add(Record);
	WriteToDisk();
	return true;
}

bool UFBFighterRosterSave::TryReplace(const FString& Id, const FFBFighterRecord& Record, FString& OutWhy)
{
	FFBFighterRecord* Existing = FindMutable(Id);
	if (!Existing)
	{
		OutWhy = TEXT("That saved fighter was not found.");
		return false;
	}
	if (!FFBFighterBalance::CanRetireInto(*Existing, Record.Ratings, OutWhy))
	{
		return false;
	}
	const int32 Refund = FFBFighterBalance::RefundedCost(Existing->Ratings, Record.Ratings);
	Existing->Ratings = Record.Ratings;
	Existing->DisplayName = Record.DisplayName;
	Existing->Division = Record.Division;
	Existing->WeightClass = FFBFighterBalance::ClassForDivision(Record.Division);
	Existing->Stance = Record.Stance;
	FFBPresentation Look = Record.Look;
	FFBBoxerCustomization::Sanitize(Look);
	if (!FFBBoxerCustomization::Validate(Look, FFBFighterBalance::ClassForDivision(Record.Division), OutWhy))
	{
		return false;
	}
	Existing->Look = Look;
	if (Existing->bFoughtOnce && Refund > 0)
	{
		Existing->RetrainBudget = FMath::Max(0, Existing->RetrainBudget - Refund);
	}
	WriteToDisk();
	return true;
}

void UFBFighterRosterSave::Remove(const FString& Id)
{
	Created.RemoveAll([&](const FFBFighterRecord& Record) { return Record.Id == Id; });
	WriteToDisk();
}

FFBFighterRecord* UFBFighterRosterSave::FindMutable(const FString& Id)
{
	for (FFBFighterRecord& Record : Created)
	{
		if (Record.Id == Id)
		{
			return &Record;
		}
	}
	return nullptr;
}
