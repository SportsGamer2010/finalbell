#include "UI/FBFightMeters.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// design_system.json. Body and stun are extra so the three pools stay readable
	// beside the one danger red and the one stamina gold.
	const FLinearColor PanelColor(0.110f, 0.129f, 0.157f, 0.90f);
	const FLinearColor TextColor(0.957f, 0.945f, 0.918f, 1.f);
	const FLinearColor MutedColor(0.655f, 0.627f, 0.580f, 1.f);
	const FLinearColor TrackColor(0.227f, 0.204f, 0.173f, 1.f);
	const FLinearColor StaminaColor(0.839f, 0.627f, 0.235f, 1.f);
	const FLinearColor HeadColor(0.769f, 0.286f, 0.227f, 1.f);
	const FLinearColor BodyColor(0.549f, 0.353f, 0.196f, 1.f);
	const FLinearColor StunColor(0.831f, 0.722f, 0.545f, 1.f);
	const FLinearColor BlueCorner(0.122f, 0.294f, 0.600f, 1.f);
	const FLinearColor RedCorner(0.557f, 0.165f, 0.165f, 1.f);

	const FSlateBrush* White()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}
}

void SFBFightMeters::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(28.f, 0.f, 0.f, 28.f))
		[
			MakePanel(BlueCorner, LocalName, LocalStamina, LocalHead, LocalBody, LocalStun)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 28.f, 28.f))
		[
			MakePanel(RedCorner, OtherNameText, OtherStamina, OtherHead, OtherBody, OtherStun)
		]
	];

	if (LocalName.IsValid())
	{
		LocalName->SetText(FText::FromString(TEXT("YOU")));
	}
	if (OtherNameText.IsValid())
	{
		OtherNameText->SetText(FText::FromString(TEXT("OPPONENT")));
	}
}

TSharedRef<SWidget> SFBFightMeters::MakeRow(const FString& Label, const FLinearColor& FillColor, FMeter& OutMeter)
{
	OutMeter.Fill = SNew(SBox)
		.WidthOverride(0.f)
		.HeightOverride(10.f)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(FillColor)
		];

	OutMeter.Value = SNew(STextBlock)
		.Text(FText::FromString(TEXT("0")))
		.ColorAndOpacity(TextColor)
		.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11));

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 3.f, 8.f, 3.f)
		[
			SNew(SBox)
			.WidthOverride(64.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(MutedColor)
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(TrackColor)
			.Padding(0.f)
			[
				SNew(SBox)
				.WidthOverride(TrackWidth)
				.HeightOverride(10.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Fill)
					[
						OutMeter.Fill.ToSharedRef()
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(36.f)
			[
				OutMeter.Value.ToSharedRef()
			]
		];
}

TSharedRef<SWidget> SFBFightMeters::MakePanel(const FLinearColor& NameColor, TSharedPtr<STextBlock>& OutName, FMeter& Stamina, FMeter& Head, FMeter& Body, FMeter& Stun)
{
	OutName = SNew(STextBlock)
		.Text(FText::FromString(TEXT("—")))
		.ColorAndOpacity(NameColor)
		.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16));

	// Oswald and Source Sans are named in the design system and are not imported.
	// The engine font is the stand-in until those families are added.
	return SNew(SBorder)
		.BorderImage(White())
		.BorderBackgroundColor(PanelColor)
		.Padding(FMargin(14.f, 12.f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				OutName.ToSharedRef()
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeRow(TEXT("STAMINA"), StaminaColor, Stamina)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeRow(TEXT("HEAD"), HeadColor, Head)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeRow(TEXT("BODY"), BodyColor, Body)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeRow(TEXT("STUN"), StunColor, Stun)
			]
		];
}

void SFBFightMeters::ApplyMeter(const FMeter& Meter, float Current, float MaxValue) const
{
	const float SafeMax = FMath::Max(MaxValue, 1.f);
	const float Clamped = FMath::Clamp(Current, 0.f, SafeMax);
	if (Meter.Fill.IsValid())
	{
		Meter.Fill->SetWidthOverride(TrackWidth * (Clamped / SafeMax));
	}
	if (Meter.Value.IsValid())
	{
		Meter.Value->SetText(FText::AsNumber(FMath::RoundToInt(Clamped)));
	}
}

void SFBFightMeters::Refresh(const AFBBoxerCharacter* Local, const AFBBoxerCharacter* Other, const FString& OtherLabel)
{
	auto Paint = [this](const AFBBoxerCharacter* Boxer, const FMeter& Stamina, const FMeter& Head, const FMeter& Body, const FMeter& Stun)
	{
		float Energy = 0.f;
		float EnergyMax = 100.f;
		float HeadValue = 0.f;
		float HeadMax = 100.f;
		float BodyValue = 0.f;
		float BodyMax = 100.f;
		float StunValue = 0.f;
		float StunMax = 100.f;
		if (Boxer)
		{
			HeadValue = Boxer->GetHeadDamage();
			BodyValue = Boxer->GetBodyDamage();
			StunValue = Boxer->GetStun();
			HeadMax = Boxer->MaxHeadDamage;
			BodyMax = Boxer->MaxBodyDamage;
			StunMax = Boxer->MaxStun;
			if (const UFBCombatComponent* Combat = Boxer->GetCombatComponent())
			{
				Energy = Combat->GetBurstEnergy();
				EnergyMax = Combat->MaxBurstEnergy;
			}
		}
		ApplyMeter(Stamina, Energy, EnergyMax);
		ApplyMeter(Head, HeadValue, HeadMax);
		ApplyMeter(Body, BodyValue, BodyMax);
		ApplyMeter(Stun, StunValue, StunMax);
	};

	if (LocalName.IsValid())
	{
		const FString LocalLabel = (Local && !Local->GetFighterDisplayName().IsEmpty()) ? Local->GetFighterDisplayName() : TEXT("YOU");
		LocalName->SetText(FText::FromString(LocalLabel));
		LocalName->SetColorAndOpacity(Local && Local->CornerSkin == EFBCornerSkin::RedCorner ? RedCorner : BlueCorner);
	}
	if (OtherNameText.IsValid())
	{
		OtherNameText->SetText(FText::FromString(OtherLabel));
		OtherNameText->SetColorAndOpacity(Other && Other->CornerSkin == EFBCornerSkin::BlueCorner ? BlueCorner : RedCorner);
	}

	Paint(Local, LocalStamina, LocalHead, LocalBody, LocalStun);
	Paint(Other, OtherStamina, OtherHead, OtherBody, OtherStun);
}
