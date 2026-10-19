// Final Bell — corner meters for the local fighter and the opponent.
// Slate, not a Widget Blueprint. The colors come from Tools/AI/config/design_system.json.
// The bars read replicated pools. They do not decide damage or stamina.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AFBBoxerCharacter;
class SBox;
class STextBlock;

class SFBFightMeters : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFBFightMeters) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Why: the pools already exist on the boxers. This copies them onto the bars
	 * each frame so a punch, a block, and a tired gas tank are visible.
	 * What: left panel is the local fighter. Right panel is the other fighter,
	 * or a named stand-in when the mode has no opponent.
	 */
	void Refresh(const AFBBoxerCharacter* Local, const AFBBoxerCharacter* Other, const FString& OtherName);

private:
	struct FMeter
	{
		TSharedPtr<SBox> Fill;
		TSharedPtr<STextBlock> Value;
	};

	TSharedRef<SWidget> MakePanel(const FLinearColor& NameColor, TSharedPtr<STextBlock>& OutName, FMeter& Stamina, FMeter& Head, FMeter& Body, FMeter& Stun);
	TSharedRef<SWidget> MakeRow(const FString& Label, const FLinearColor& FillColor, FMeter& OutMeter);
	void ApplyMeter(const FMeter& Meter, float Current, float MaxValue) const;

	static constexpr float TrackWidth = 168.f;

	TSharedPtr<STextBlock> LocalName;
	TSharedPtr<STextBlock> OtherNameText;
	FMeter LocalStamina;
	FMeter LocalHead;
	FMeter LocalBody;
	FMeter LocalStun;
	FMeter OtherStamina;
	FMeter OtherHead;
	FMeter OtherBody;
	FMeter OtherStun;
};
