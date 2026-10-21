// Pure rules for fighter-select ownership. No widgets, so the automation test can call them.
#pragma once

#include "CoreMinimal.h"
#include "Combat/FBCombatTypes.h"

struct FFBSelectNav
{
	/** Move inside one panel. Delta wraps in that panel and cannot enter another side. */
	static int32 Step(const TArray<int32>& Sides, int32 Focus, int32 Delta, int32 Side)
	{
		TArray<int32> Rows;
		for (int32 Index = 0; Index < Sides.Num(); ++Index)
		{
			if (Sides[Index] == Side)
			{
				Rows.Add(Index);
			}
		}
		if (Rows.Num() == 0 || Delta == 0)
		{
			return Focus;
		}
		int32 Slot = Rows.IndexOfByKey(Focus);
		if (Slot == INDEX_NONE)
		{
			Slot = Delta < 0 ? Rows.Num() - 1 : 0;
		}
		else
		{
			Slot = (Slot + (Delta > 0 ? 1 : -1) + Rows.Num()) % Rows.Num();
		}
		return Rows[Slot];
	}

	static int32 FirstOnSide(const TArray<int32>& Sides, int32 Side)
	{
		for (int32 Index = 0; Index < Sides.Num(); ++Index)
		{
			if (Sides[Index] == Side)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Which panel this device may change.
	 * INDEX_NONE means the device is ignored.
	 * Local controller index is not a network seat: online guests pass MatchSeat 1 with DeviceSeat 0.
	 */
	static int32 PanelForDevice(EFBMatchSeatMode Mode, int32 MatchSeat, int32 ActivePanel, bool bSecondJoined, int32 DeviceSeat)
	{
		if (Mode == EFBMatchSeatMode::LocalVersus)
		{
			if (DeviceSeat <= 0)
			{
				return 0;
			}
			if (DeviceSeat == 1 && bSecondJoined)
			{
				return 1;
			}
			return INDEX_NONE;
		}
		if (Mode == EFBMatchSeatMode::Online)
		{
			if (DeviceSeat != 0 || MatchSeat < 0)
			{
				return INDEX_NONE;
			}
			return MatchSeat == 0 ? 0 : 1;
		}
		if (DeviceSeat != 0)
		{
			return INDEX_NONE;
		}
		return ActivePanel;
	}

	/** Pointers follow the same panels. In Local VS the shared mouse stays on the left. */
	static bool AllowsPointer(EFBMatchSeatMode Mode, int32 MatchSeat, int32 ActivePanel, int32 Side)
	{
		if (Side < 0)
		{
			return false;
		}
		if (Mode == EFBMatchSeatMode::LocalVersus)
		{
			return Side == 0;
		}
		if (Mode == EFBMatchSeatMode::Online)
		{
			if (MatchSeat < 0)
			{
				return false;
			}
			return Side == (MatchSeat == 0 ? 0 : 1);
		}
		return Side == ActivePanel;
	}
};
