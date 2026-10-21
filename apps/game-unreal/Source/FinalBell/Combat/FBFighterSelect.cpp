#include "Combat/FBSparringPlayerController.h"
#include "Input/FBGameViewportClient.h"
#include "Combat/FBSelectNav.h"
#include "Network/FBPrivateRoomSubsystem.h"
#include "Combat/FBSparringGameState.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBFighterSave.h"
#include "Combat/FBBoxerCustomization.h"
#include "Combat/FBSparringGameMode.h"
#include "Session/FBSessionSubsystem.h"
#include "UI/FBGameInstance.h"
#include "UI/FBMenuArt.h"
#include "UI/FBPlayerSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Input/Events.h"
#include "Styling/CoreStyle.h"

class SFBFighterSelectStage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFBFighterSelectStage) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(float*, Reveal)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Reveal = InArgs._Reveal;
		Paper = FBMenuArt::Paper();
		Quiet = FBMenuArt::Quiet();
		Accent = FBMenuArt::Accent();
		Line = FLinearColor(0.2275f, 0.2039f, 0.1725f, 1.f);
		ChildSlot[InArgs._Content.Widget];
		Apply(Reveal ? *Reveal : 1.f);
		SetCanTick(true);
		SetVisibility(EVisibility::SelfHitTestInvisible);
	}

	void Watch(int32* InFocus, TArray<int32> InSides, TSharedPtr<SBorder> InLeft, TSharedPtr<SBorder> InRight, TSharedPtr<SBorder> InActions, TSharedPtr<STextBlock> InLeftTitle, TSharedPtr<STextBlock> InRightTitle)
	{
		Focus = InFocus;
		Sides = MoveTemp(InSides);
		LeftFrame = InLeft;
		RightFrame = InRight;
		ActionFrame = InActions;
		LeftTitle = InLeftTitle;
		RightTitle = InRightTitle;
		ApplyFocus();
	}

	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		return FReply::Handled();
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		float Now = Reveal ? *Reveal : 1.f;
		if (Now < 1.f)
		{
			Now = FMath::Min(1.f, Now + InDeltaTime / 0.36f);
			if (Reveal)
			{
				*Reveal = Now;
			}
			Apply(Now);
		}
		ApplyFocus();
	}

private:
	void Apply(float Now)
	{
		SetRenderOpacity(Now);
		SetRenderTransform(FSlateRenderTransform(FVector2D((1.f - Now) * -36.f, 0.f)));
	}

	void ApplyFocus()
	{
		const int32 Index = Focus ? *Focus : 0;
		const int32 Side = Sides.IsValidIndex(Index) ? Sides[Index] : 0;
		auto Frame = [&](const TSharedPtr<SBorder>& Border, bool bOn)
		{
			if (Border.IsValid())
			{
				Border->SetBorderBackgroundColor(bOn ? Accent : Line);
			}
		};
		Frame(LeftFrame, Side == 0);
		Frame(RightFrame, Side == 1);
		Frame(ActionFrame, Side == 2);
		if (LeftTitle.IsValid())
		{
			LeftTitle->SetColorAndOpacity(Side == 0 ? Paper : Quiet);
		}
		if (RightTitle.IsValid())
		{
			RightTitle->SetColorAndOpacity(Side == 1 ? Paper : Quiet);
		}
	}

	float* Reveal = nullptr;
	int32* Focus = nullptr;
	TArray<int32> Sides;
	TSharedPtr<SBorder> LeftFrame;
	TSharedPtr<SBorder> RightFrame;
	TSharedPtr<SBorder> ActionFrame;
	TSharedPtr<STextBlock> LeftTitle;
	TSharedPtr<STextBlock> RightTitle;
	FLinearColor Paper;
	FLinearColor Quiet;
	FLinearColor Accent;
	FLinearColor Line;
};

class SFBCreatorViewport : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SFBCreatorViewport) {}
		SLATE_ARGUMENT(AFBSparringPlayerController*, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Controller = InArgs._Controller;
		const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		TSharedPtr<STextBlock> Measure;
		SBorder::Construct(SBorder::FArguments()
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.01f))
			.Padding(FMargin(16.f, 14.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DRAG TO ROTATE      WHEEL TO ZOOM")))
					.Font(FBMenuArt::PosterFont(12, 40))
					.ColorAndOpacity(FLinearColor(0.93f, 0.90f, 0.84f, 0.92f))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage(White)
					.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.01f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(Measure, STextBlock)
					.Text(FText::FromString(Controller.IsValid() ? Controller->GetCreatorMeasureText() : FString()))
					.Font(FBMenuArt::PosterFont(16, 22))
					.ColorAndOpacity(FLinearColor(0.93f, 0.90f, 0.84f))
					.AutoWrapText(true)
				]
			]);
		if (Controller.IsValid())
		{
			Controller->SetCreatorMeasure(Measure);
		}
	}

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Controller.IsValid())
		{
			bDrag = true;
			Last = Event.GetScreenSpacePosition();
			Controller->SetCreatorDrag(true);
			return FReply::Handled().CaptureMouse(AsShared());
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		bDrag = false;
		if (Controller.IsValid())
		{
			Controller->SetCreatorDrag(false);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}

	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (!bDrag || !Controller.IsValid())
		{
			return FReply::Unhandled();
		}
		const FVector2D Now = Event.GetScreenSpacePosition();
		const FVector2D Delta = Now - Last;
		Last = Now;
		Controller->AddCreatorOrbit(Delta.X * 0.35f, -Delta.Y * 0.18f);
		return FReply::Handled();
	}

	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Controller.IsValid())
		{
			Controller->AddCreatorZoom(Event.GetWheelDelta() * -0.08f);
		}
		return FReply::Handled();
	}

private:
	TWeakObjectPtr<AFBSparringPlayerController> Controller;
	bool bDrag = false;
	FVector2D Last = FVector2D::ZeroVector;
};

namespace
{
	FSlateFontInfo MenuFont(int32 Size)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
		return Font;
	}

	const FButtonStyle& ClearButton()
	{
		static FButtonStyle Style = []()
		{
			FButtonStyle Out;
			const FSlateColorBrush Clear(FLinearColor::Transparent);
			Out.SetNormal(Clear);
			Out.SetHovered(Clear);
			Out.SetPressed(Clear);
			Out.SetDisabled(Clear);
			Out.NormalPadding = FMargin(0.f);
			Out.PressedPadding = FMargin(0.f);
			return Out;
		}();
		return Style;
	}

	FString ShortAttr(EFBAttributeId Id)
	{
		switch (Id)
		{
		case EFBAttributeId::Chin: return TEXT("Chin");
		case EFBAttributeId::Body: return TEXT("Body");
		case EFBAttributeId::Cut: return TEXT("Cut");
		case EFBAttributeId::Bruise: return TEXT("Bruise");
		case EFBAttributeId::Guarding: return TEXT("Guard");
		case EFBAttributeId::Courage: return TEXT("Courage");
		case EFBAttributeId::BlockHealth: return TEXT("Block");
		case EFBAttributeId::BlockRegen: return TEXT("Guard recovery");
		case EFBAttributeId::Reflexes: return TEXT("Reflexes");
		case EFBAttributeId::Footwork: return TEXT("Feet");
		case EFBAttributeId::PunchSpeed: return TEXT("Hand speed");
		case EFBAttributeId::Accuracy: return TEXT("Accuracy");
		case EFBAttributeId::Stamina: return TEXT("Stamina");
		case EFBAttributeId::StaminaRegen: return TEXT("Gas refill");
		case EFBAttributeId::StaminaRecovery: return TEXT("Round gas");
		case EFBAttributeId::Conditioning: return TEXT("Conditioning");
		case EFBAttributeId::Endurance: return TEXT("Endurance");
		case EFBAttributeId::LeftHand: return TEXT("Left hand");
		case EFBAttributeId::RightHand: return TEXT("Right hand");
		default: return TEXT("Rating");
		}
	}

	struct FStyleMarks
	{
		EFBAttributeId High[3] = { EFBAttributeId::Chin, EFBAttributeId::Chin, EFBAttributeId::Chin };
		EFBAttributeId Low[2] = { EFBAttributeId::Chin, EFBAttributeId::Chin };
		bool bFound = false;
	};

	FStyleMarks MarksFor(const FString& Style, const TArray<int32>& Ratings)
	{
		FStyleMarks Marks;
		auto Set = [&](EFBAttributeId A, EFBAttributeId B, EFBAttributeId C, EFBAttributeId D, EFBAttributeId E)
		{
			Marks.High[0] = A;
			Marks.High[1] = B;
			Marks.High[2] = C;
			Marks.Low[0] = D;
			Marks.Low[1] = E;
			Marks.bFound = true;
		};
		if (Style == TEXT("Slugger")) Set(EFBAttributeId::RightHand, EFBAttributeId::LeftHand, EFBAttributeId::Chin, EFBAttributeId::Footwork, EFBAttributeId::BlockRegen);
		else if (Style == TEXT("In-fighter")) Set(EFBAttributeId::Conditioning, EFBAttributeId::Endurance, EFBAttributeId::Body, EFBAttributeId::Footwork, EFBAttributeId::Reflexes);
		else if (Style == TEXT("Counterpuncher")) Set(EFBAttributeId::Reflexes, EFBAttributeId::Accuracy, EFBAttributeId::Guarding, EFBAttributeId::Stamina, EFBAttributeId::Endurance);
		else if (Style == TEXT("Boxer-puncher")) Set(EFBAttributeId::LeftHand, EFBAttributeId::RightHand, EFBAttributeId::Accuracy, EFBAttributeId::BlockRegen, EFBAttributeId::Conditioning);
		else if (Style == TEXT("Swarmer")) Set(EFBAttributeId::PunchSpeed, EFBAttributeId::Endurance, EFBAttributeId::Stamina, EFBAttributeId::Chin, EFBAttributeId::BlockRegen);
		else if (Style == TEXT("Out-boxer")) Set(EFBAttributeId::Footwork, EFBAttributeId::Accuracy, EFBAttributeId::PunchSpeed, EFBAttributeId::RightHand, EFBAttributeId::Chin);
		else if (Style == TEXT("High guard")) Set(EFBAttributeId::Guarding, EFBAttributeId::BlockHealth, EFBAttributeId::BlockRegen, EFBAttributeId::Footwork, EFBAttributeId::PunchSpeed);
		else if (Style == TEXT("Southpaw")) Set(EFBAttributeId::LeftHand, EFBAttributeId::Footwork, EFBAttributeId::Accuracy, EFBAttributeId::Bruise, EFBAttributeId::BlockRegen);
		else if (Style == TEXT("Brawler")) Set(EFBAttributeId::Chin, EFBAttributeId::Body, EFBAttributeId::Courage, EFBAttributeId::Footwork, EFBAttributeId::Accuracy);
		else if (Style == TEXT("Jab technician")) Set(EFBAttributeId::LeftHand, EFBAttributeId::Accuracy, EFBAttributeId::Reflexes, EFBAttributeId::Body, EFBAttributeId::Conditioning);
		else if (Style == TEXT("Power puncher")) Set(EFBAttributeId::RightHand, EFBAttributeId::LeftHand, EFBAttributeId::Chin, EFBAttributeId::Stamina, EFBAttributeId::Footwork);
		else if (Style == TEXT("Body puncher")) Set(EFBAttributeId::Body, EFBAttributeId::Conditioning, EFBAttributeId::Endurance, EFBAttributeId::Chin, EFBAttributeId::Reflexes);
		else if (Style == TEXT("Slick boxer")) Set(EFBAttributeId::Reflexes, EFBAttributeId::Footwork, EFBAttributeId::Accuracy, EFBAttributeId::RightHand, EFBAttributeId::Body);
		else if (Style == TEXT("Inside fighter")) Set(EFBAttributeId::Conditioning, EFBAttributeId::Body, EFBAttributeId::Endurance, EFBAttributeId::Footwork, EFBAttributeId::Reflexes);
		else if (Style == TEXT("Speedster")) Set(EFBAttributeId::PunchSpeed, EFBAttributeId::Footwork, EFBAttributeId::Reflexes, EFBAttributeId::Chin, EFBAttributeId::RightHand);
		else if (Style == TEXT("Peek-a-boo")) Set(EFBAttributeId::Guarding, EFBAttributeId::BlockHealth, EFBAttributeId::Courage, EFBAttributeId::Footwork, EFBAttributeId::Accuracy);
		else if (Ratings.Num() == FFBFighterBalance::AttributeCount)
		{
			TArray<int32> Order;
			for (int32 Index = 0; Index < Ratings.Num(); ++Index)
			{
				Order.Add(Index);
			}
			Order.Sort([&](int32 A, int32 B)
			{
				return Ratings[A] == Ratings[B] ? A < B : Ratings[A] > Ratings[B];
			});
			Marks.High[0] = static_cast<EFBAttributeId>(Order[0]);
			Marks.High[1] = static_cast<EFBAttributeId>(Order[1]);
			Marks.High[2] = static_cast<EFBAttributeId>(Order[2]);
			Marks.Low[0] = static_cast<EFBAttributeId>(Order[Order.Num() - 1]);
			Marks.Low[1] = static_cast<EFBAttributeId>(Order[Order.Num() - 2]);
			Marks.bFound = true;
		}
		return Marks;
	}

	FString StyleLabel(const FFBFighterRecord& Fighter)
	{
		return Fighter.Style.IsEmpty() ? TEXT("Custom") : Fighter.Style;
	}

	FString ClassBlurb(EFBWeightClassId ClassId)
	{
		switch (ClassId)
		{
		case EFBWeightClassId::Lightweight: return TEXT("Bantam, feather, and lightweight");
		case EFBWeightClassId::Heavyweight: return TEXT("Cruiserweight and heavyweight");
		default: return TEXT("Welter, middle, and light heavy");
		}
	}

	FLinearColor StandInSkin(const FFBPresentation& Look)
	{
		const float Light = FMath::Lerp(0.16f, 0.88f, FMath::Clamp(Look.SkinU, 0.f, 1.f));
		const FLinearColor Cool(Light * 0.82f, Light * 0.64f, Light * 0.52f);
		const FLinearColor Warm(Light, Light * 0.52f, Light * 0.34f);
		return FMath::Lerp(Cool, Warm, FMath::Clamp(Look.SkinV, 0.f, 1.f));
	}

	FLinearColor GearColor(int32 Index)
	{
		switch (Index % 6)
		{
		case 0: return FLinearColor(0.55f, 0.08f, 0.08f);
		case 1: return FLinearColor(0.08f, 0.18f, 0.55f);
		case 2: return FLinearColor(0.08f, 0.08f, 0.08f);
		case 3: return FLinearColor(0.72f, 0.55f, 0.16f);
		case 4: return FLinearColor(0.85f, 0.85f, 0.82f);
		default: return FLinearColor(0.10f, 0.38f, 0.16f);
		}
	}

	/** Skin, hair, trunks, and gloves already stored on the fighter. The mannequin uses the same colors. */
	TSharedRef<SWidget> PortraitStandIn(const FFBFighterRecord& Fighter)
	{
		const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		const FLinearColor Skin = StandInSkin(Fighter.Look);
		const FLinearColor Hair = FMath::Lerp(
			FLinearColor(0.08f, 0.07f, 0.06f),
			FLinearColor(0.62f, 0.62f, 0.60f),
			FMath::Clamp(Fighter.Look.HairGray, 0.f, 1.f));
		const FLinearColor Trunks = GearColor(Fighter.Look.TrunksColor);
		const FLinearColor Gloves = GearColor(Fighter.Look.GloveColor);
		float HairHeight = 8.f;
		switch (Fighter.Look.Hair)
		{
		case EFBHairStyle::Bald: HairHeight = 0.f; break;
		case EFBHairStyle::Crop:
		case EFBHairStyle::Fade: HairHeight = 8.f; break;
		case EFBHairStyle::Short: HairHeight = 12.f; break;
		default: HairHeight = 16.f; break;
		}

		TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);
		Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(36.f).HeightOverride(HairHeight > 0.f ? HairHeight : 6.f)
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(HairHeight > 0.f ? Hair : Skin)
			]
		];
		Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(32.f).HeightOverride(32.f)
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(Skin)
			]
		];
		if (Fighter.Look.Beard > 0)
		{
			Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox).WidthOverride(20.f).HeightOverride(6.f)
				[
					SNew(SBorder).BorderImage(White).BorderBackgroundColor(Hair)
				]
			];
		}
		Column->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
		[
			SNew(SBox).WidthOverride(56.f).HeightOverride(34.f)
			[
				SNew(SBorder)
				.BorderImage(White)
				.BorderBackgroundColor(Skin)
				.Padding(0.f)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox).HeightOverride(12.f)
					[
						SNew(SBorder).BorderImage(White).BorderBackgroundColor(Trunks)
					]
				]
			]
		];
		Column->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(56.f).HeightOverride(4.f)
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(Gloves)
			]
		];

		return SNew(SBox).WidthOverride(76.f).HeightOverride(108.f)
		[
			SNew(SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor(0.07f, 0.08f, 0.09f, 1.f))
			.Padding(8.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				Column
			]
		];
	}

	void WeakPhrases(const FFBFighterRecord& Fighter, FString& First, FString& Second)
	{
		const FStyleMarks Marks = MarksFor(Fighter.Style, Fighter.Ratings);
		auto Phrase = [&](EFBAttributeId Id)
		{
			const int32 Index = static_cast<int32>(Id);
			const int32 Value = Fighter.Ratings.IsValidIndex(Index) ? Fighter.Ratings[Index] : 0;
			return FString::Printf(TEXT("%s %d"), *ShortAttr(Id).ToLower(), Value);
		};
		if (!Marks.bFound)
		{
			First = TEXT("no rating");
			Second = TEXT("no rating");
			return;
		}
		First = Phrase(Marks.Low[0]);
		Second = Phrase(Marks.Low[1]);
	}

	bool IsLowRating(const FFBFighterRecord& Fighter, EFBAttributeId Id)
	{
		const FStyleMarks Marks = MarksFor(Fighter.Style, Fighter.Ratings);
		return Marks.bFound && (Marks.Low[0] == Id || Marks.Low[1] == Id);
	}

	FString FormatKeys(const TCHAR* Pattern, const FString& A, const FString& B)
	{
		return FString::Format(Pattern, FStringFormatOrderedArguments{ A, B });
	}

	FString FormatThreat(const TCHAR* Pattern, const FString& Style, const FString& A, const FString& B)
	{
		return FString::Format(Pattern, FStringFormatOrderedArguments{ Style, A, B });
	}

	FString ArticleStyle(const FString& Style)
	{
		const TCHAR First = Style.IsEmpty() ? TEXT('x') : FChar::ToLower(Style[0]);
		const bool bAn = First == TEXT('a') || First == TEXT('e') || First == TEXT('i') || First == TEXT('o') || First == TEXT('u');
		return FString(bAn ? TEXT("an ") : TEXT("a ")) + Style.ToLower();
	}

	/** How to approach this style. {0} and {1} are its two low ratings. */
	FString KeysToApproach(const FFBFighterRecord& Opponent)
	{
		if (Opponent.Ratings.Num() != FFBFighterBalance::AttributeCount)
		{
			return TEXT("Pick an opponent in this class to see the keys.");
		}
		FString First;
		FString Second;
		WeakPhrases(Opponent, First, Second);
		const FString& Style = Opponent.Style;
		const TCHAR* Pattern = TEXT("Work the two low spots on this build: {0} and {1}.\nLeave the ratings they paid for alone.\nThat hole is the bout.");
		if (Style == TEXT("Slugger")) Pattern = TEXT("Make the slugger miss, then step off the rear hand.\nHe gives up {0} and {1}, so the guard stays down and the feet do not chase.\nA slugger loses the fight he has to walk down.");
		else if (Style == TEXT("Brawler")) Pattern = TEXT("Stay off the line. A brawler walks through fire and swings at where you were.\nHe gives up {0} and {1}.\nThe punches have to hit air.");
		else if (Style == TEXT("In-fighter")) Pattern = TEXT("Circle an in-fighter. The body work only lands if you stand there.\nHe gives up {0} and {1}.\nA mover who does not trade walks him out of the fight.");
		else if (Style == TEXT("Jab technician")) Pattern = TEXT("Get past the jab and work the body. That is the hole.\nHe gives up {0} and {1}.\nHe loses when the fight stays in the pocket.");
		else if (Style == TEXT("Counterpuncher")) Pattern = TEXT("Make him work without handing him the clean miss.\nHe gives up {0} and {1}, so a long firefight empties him.\nThe reply is already there if you load up and miss.");
		else if (Style == TEXT("Power puncher")) Pattern = TEXT("Take it long. Leave the early bomb trade alone.\nHe gives up {0} and {1}.\nThe chin holds. The gas and the feet do not.");
		else if (Style == TEXT("Boxer-puncher")) Pattern = TEXT("Hold the center and make him miss.\nHe gives up {0} and {1}.\nThe hands are real. The guard and the late rounds are not.");
		else if (Style == TEXT("Body puncher")) Pattern = TEXT("Keep the head off the line and counter upstairs.\nHe gives up {0} and {1}.\nEverything he throws is to the body.");
		else if (Style == TEXT("Swarmer")) Pattern = TEXT("Answer clean, or keep the outside. Do not back straight up.\nHe gives up {0} and {1}.\nThe hands keep coming, and the chin pays for it.");
		else if (Style == TEXT("Slick boxer")) Pattern = TEXT("Cut the ring and touch the body. He has no bomb.\nHe gives up {0} and {1}.\nHe wins while the ring stays big.");
		else if (Style == TEXT("Out-boxer")) Pattern = TEXT("Cut the out-boxer off and make him trade. The jab is the fight until then.\nHe gives up {0} and {1}.\nAn out-boxer with no bomb loses in the pocket.");
		else if (Style == TEXT("Inside fighter")) Pattern = TEXT("Keep the ring big and answer from the outside.\nHe gives up {0} and {1}.\nHe wants the body once he cuts you off.");
		else if (Style == TEXT("High guard")) Pattern = TEXT("Make him move, or go to the body. The gloves are not the target.\nHe gives up {0} and {1}.\nThe shell is the weapon. The feet and the hands are not.");
		else if (Style == TEXT("Speedster")) Pattern = TEXT("Time one clean shot. Leave the reaching alone.\nHe gives up {0} and {1}.\nHe loses if he is hit clean or made to trade the right hand.");
		else if (Style == TEXT("Southpaw")) Pattern = TEXT("Step outside the lead foot. Stay off the rear left.\nHe gives up {0} and {1}.\nHe loses when he stands square in front of a puncher.");
		else if (Style == TEXT("Peek-a-boo")) Pattern = TEXT("Make him chase, or pick a shot. Leave the shell alone.\nHe gives up {0} and {1}.\nCourage keeps him there. The feet and the aim do not.");
		return FormatKeys(Pattern, First, Second);
	}

	/** How Opponent's style tries to beat Player. {0} is the player's style. {1} and {2} are the player's low ratings. */
	FString KeysTheyUse(const FFBFighterRecord& Opponent, const FFBFighterRecord& Player)
	{
		if (Opponent.Ratings.Num() != FFBFighterBalance::AttributeCount || Player.Ratings.Num() != FFBFighterBalance::AttributeCount)
		{
			return TEXT("Pick both fighters to see how this style hunts the other.");
		}
		FString First;
		FString Second;
		WeakPhrases(Player, First, Second);
		const FString Styled = ArticleStyle(StyleLabel(Player));
		const FString& Style = Opponent.Style;
		const TCHAR* Pattern = TEXT("They look for the holes in {0}: {1} and {2}.\nThat is the fight they want.\nThe ratings they paid for are the bait.");
		if (Style == TEXT("Slugger")) Pattern = TEXT("The slugger wants one clean rear hand against {0}.\nHe walks through shots and hunts {1} and {2}.\nHe loses that plan when he has to chase.");
		else if (Style == TEXT("Brawler")) Pattern = TEXT("The brawler marches in and swings at {0}.\nHe hunts {1} and {2} and does not need to be precise if you stand still.\nHe loses when the punches hit air.");
		else if (Style == TEXT("In-fighter")) Pattern = TEXT("The in-fighter walks through {0} and works the body.\nHe hunts {1} and {2} and stays in the pocket.\nA mover who will not trade takes him out of it.");
		else if (Style == TEXT("Jab technician")) Pattern = TEXT("The jab technician keeps {0} on the end of the jab.\nHe looks for {1} and {2} and will not stay in the pocket.\nThe body, and a long night, are how he loses.");
		else if (Style == TEXT("Counterpuncher")) Pattern = TEXT("The counterpuncher makes {0} miss, then the reply is already there.\nHe looks for {1} and {2}.\nA firefight empties the tank he gave up.");
		else if (Style == TEXT("Power puncher")) Pattern = TEXT("The power puncher wants {0} standing in front of both hands.\nThe chin holds while he hunts {1} and {2}.\nHe fades if he has to chase.");
		else if (Style == TEXT("Boxer-puncher")) Pattern = TEXT("The boxer-puncher takes the center from {0} and picks with both hands.\nHe looks for {1} and {2}.\nHe loses if he gives that center up.");
		else if (Style == TEXT("Body puncher")) Pattern = TEXT("The body puncher bends {0} and takes the tank.\nHe hunts {1} and {2} downstairs.\nA clean counter upstairs is the hole.");
		else if (Style == TEXT("Swarmer")) Pattern = TEXT("The swarmer smothers {0} and keeps the hands going.\nHe hunts {1} and {2} on the way in.\nA clean counter, or the outside, beats the chin.");
		else if (Style == TEXT("Slick boxer")) Pattern = TEXT("The slick boxer makes the ring big against {0}.\nHe lands from range and looks for {1} and {2}.\nHe has no bomb and no body work.");
		else if (Style == TEXT("Out-boxer")) Pattern = TEXT("The out-boxer stays long against {0} and jabs.\nHe looks for {1} and {2} and will not trade bombs.\nHe loses if he is cut off.");
		else if (Style == TEXT("Inside fighter")) Pattern = TEXT("The inside fighter cuts {0} off and stays on the body.\nHe hunts {1} and {2} once the ring is small.\nLong feet and a counter take him out of it.");
		else if (Style == TEXT("High guard")) Pattern = TEXT("The high guard shells up and makes {0} miss on the gloves.\nHe waits on {1} and {2}.\nHe loses if he has to chase or trade power.");
		else if (Style == TEXT("Speedster")) Pattern = TEXT("The speedster beats {0} with feet and hand speed, then is gone.\nHe looks for {1} and {2} and does not trade the right hand.\nOne clean shot is how he loses.");
		else if (Style == TEXT("Southpaw")) Pattern = TEXT("The southpaw fights {0} off the angle, rear left hand first.\nHe looks for {1} and {2} from that side.\nHe loses if he stands square in front of a puncher.");
		else if (Style == TEXT("Peek-a-boo")) Pattern = TEXT("The peek-a-boo shells and walks through {0}.\nCourage holds while he looks for {1} and {2}.\nHe loses when he has to chase or pick a punch.");
		return FormatThreat(Pattern, Styled, First, Second);
	}

	TSharedRef<SWidget> RatingGrid(const FFBFighterRecord& Fighter, const FLinearColor& Paper, const FLinearColor& Accent, const FLinearColor& Quiet)
	{
		const FSlateFontInfo NameFont = FBMenuArt::PosterFont(11, 0);
		const FSlateFontInfo ValueFont = FBMenuArt::PosterFont(13, 0);
		TSharedRef<SVerticalBox> Left = SNew(SVerticalBox);
		TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
		const int32 Half = (FFBFighterBalance::AttributeCount + 1) / 2;
		for (int32 Attr = 0; Attr < FFBFighterBalance::AttributeCount; ++Attr)
		{
			const EFBAttributeId Id = static_cast<EFBAttributeId>(Attr);
			const int32 Value = Fighter.Ratings.IsValidIndex(Attr) ? Fighter.Ratings[Attr] : 0;
			const bool bLow = IsLowRating(Fighter, Id);
			TSharedRef<SVerticalBox> Column = Attr < Half ? Left : Right;
			Column->AddSlot().AutoHeight().Padding(0.f, 1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FFBFighterBalance::AttributeName(Id)))
					.Font(NameFont)
					.ColorAndOpacity(bLow ? Paper : Quiet)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(Value))
					.Font(ValueFont)
					.ColorAndOpacity(bLow ? Accent : Paper)
				]
			];
		}
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 10.f, 0.f)[ Left ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ Right ];
	}
}

void AFBSparringPlayerController::LoadSelectRoster()
{
	SelectRoster.Reset();
	FFBFighterBalance::GetPremades(SelectRoster);
	if (UFBFighterRosterSave* Saved = UFBFighterRosterSave::LoadOrCreate())
	{
		for (const FFBFighterRecord& Created : Saved->Created)
		{
			if (FFBFighterBalance::IsDraftLegal(Created.Ratings))
			{
				SelectRoster.Add(Created);
			}
		}
	}
	PlayerPick = FMath::Clamp(PlayerPick, 0, FMath::Max(0, SelectRoster.Num() - 1));
	OpponentPick = FMath::Clamp(OpponentPick, 0, FMath::Max(0, SelectRoster.Num() - 1));
	EnsureOpponentInClass();
}

void AFBSparringPlayerController::ShowFighterSelect()
{
	if (!IsLocalController())
	{
		return;
	}
	const bool bOpening = !bFighterSelectVisible;
	LoadSelectRoster();
	if (bCareerSelect)
	{
		for (int32 Index = 0; Index < SelectRoster.Num(); ++Index)
		{
			if (!SelectRoster[Index].bPremade)
			{
				PlayerPick = Index;
				break;
			}
		}
		bool bHasCreated = false;
		for (const FFBFighterRecord& Fighter : SelectRoster)
		{
			if (!Fighter.bPremade)
			{
				bHasCreated = true;
				break;
			}
		}
		if (!bHasCreated && SelectStatus.IsEmpty())
		{
			SelectStatus = TEXT("No created fighter yet. Save one if you want this bout on your record.");
		}
	}
	if (!bBuilderOpen)
	{
		Draft = SelectRoster.IsValidIndex(PlayerPick) ? SelectRoster[PlayerPick] : FFBFighterRecord();
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		if (SeatMode == EFBMatchSeatMode::Unset && FrontEnd->GetSeatMode() != EFBMatchSeatMode::Unset)
		{
			SeatMode = FrontEnd->GetSeatMode();
		}
	}
	if (SeatMode == EFBMatchSeatMode::Unset)
	{
		SeatMode = bCareerSelect ? EFBMatchSeatMode::VersusAI : EFBMatchSeatMode::VersusAI;
	}
	if (bOpening && IsSequentialSelect() && SelectStatus.IsEmpty())
	{
		SelectStatus = TEXT("Choose Your Fighter.");
		ActiveSide = 0;
	}
	if (bOpening && SeatMode == EFBMatchSeatMode::LocalVersus && SelectStatus.IsEmpty())
	{
		SelectStatus = TEXT("Press Start on the second controller to join the right side.");
	}
	if (bOpening && SeatMode == EFBMatchSeatMode::Online)
	{
		if (MatchSeat == 1)
		{
			SelectStatus = TEXT("You are player 2. Confirm the fighter on the right.");
		}
		else if (UGameInstance* GI = GetGameInstance())
		{
			if (UFBPrivateRoomSubsystem* Rooms = GI->GetSubsystem<UFBPrivateRoomSubsystem>())
			{
				SelectStatus = FString::Printf(TEXT("You are player 1. Share %s. Confirm the fighter on the left."), *Rooms->GetShareToken());
			}
		}
	}
	if (UFBGameViewportClient* Pads = GetWorld() ? Cast<UFBGameViewportClient>(GetWorld()->GetGameViewport()) : nullptr)
	{
		Pads->SetSelectPadMode(IsSequentialSelect(), SeatMode == EFBMatchSeatMode::LocalVersus);
	}
	if (SeatMode == EFBMatchSeatMode::LocalVersus)
	{
		EnsureSecondLocalPlayer();
	}
	if (bOpening)
	{
		SelectReveal = 0.f;
		if (const UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
		{
			SelectDifficulty = FFBTactical::SanitizeDifficulty(Saved->AIDifficulty);
			bCreatorIdleSpin = !Saved->bReducedMotion;
			if (Saved->bReducedMotion)
			{
				SelectReveal = 1.f;
			}
		}
	}
	bFighterSelectVisible = true;
	bShowMouseCursor = true;
	SuspendFightMapping();
	RebuildFighterSelect();
}

void AFBSparringPlayerController::HideFighterSelect()
{
	if (FighterSelectWidget.IsValid() && GEngine && LocalViewport())
	{
		LocalViewport()->RemoveViewportWidgetContent(FighterSelectWidget.ToSharedRef());
	}
	FighterSelectWidget.Reset();
	SelectRows.Reset();
	SelectActions.Reset();
	CreatorMeasure.Reset();
	ShutdownCreatorCamera();
	bFighterSelectVisible = false;
	if (UFBGameViewportClient* Pads = GetWorld() ? Cast<UFBGameViewportClient>(GetWorld()->GetGameViewport()) : nullptr)
	{
		Pads->SetSelectPadMode(false, false);
	}
	bShowMouseCursor = false;
	if (!IsMenuTakingInput())
	{
		RestoreFightMapping();
	}
}

void AFBSparringPlayerController::SaveCurrentDraft()
{
	FString Why;
	Draft.RulesetId = TEXT("CompetitiveFair");
	Draft.RulesetVersion = FFBFighterBalance::RulesetVersion;
	Draft.WeightClass = FFBFighterBalance::ClassForDivision(Draft.Division);
	Draft.bPremade = false;
	if (Draft.DisplayName.IsEmpty())
	{
		Draft.DisplayName = TEXT("Your Fighter");
	}
	UFBFighterRosterSave* Roster = UFBFighterRosterSave::LoadOrCreate();
	if (!Roster)
	{
		SelectStatus = TEXT("Could not open the save.");
		return;
	}
	if (Draft.Id.IsEmpty() || Draft.Id.StartsWith(TEXT("premade_")))
	{
		Draft.Id.Reset();
		Draft.bFoughtOnce = false;
		Draft.RetrainBudget = 0;
		if (!Roster->TryAdd(Draft, Why))
		{
			SelectStatus = Why;
			return;
		}
		Draft = Roster->Created.Last();
		SelectStatus = TEXT("Saved. Ten is the limit for created fighters.");
	}
	else if (!Roster->TryReplace(Draft.Id, Draft, Why))
	{
		SelectStatus = Why;
		return;
	}
	else
	{
		SelectStatus = TEXT("Updated.");
	}
	LoadSelectRoster();
	for (int32 Index = 0; Index < SelectRoster.Num(); ++Index)
	{
		if (SelectRoster[Index].Id == Draft.Id)
		{
			PlayerPick = Index;
			Draft = SelectRoster[Index];
			break;
		}
	}
}

void AFBSparringPlayerController::ConfirmCompetitiveBout()
{
	if (bBoutStartPending)
	{
		return;
	}
	if (!SelectRoster.IsValidIndex(PlayerPick) || !SelectRoster.IsValidIndex(OpponentPick))
	{
		SelectStatus = TEXT("Pick two fighters.");
		RebuildFighterSelect();
		return;
	}
	if (UsesSplitSeats())
	{
		if (SeatMode == EFBMatchSeatMode::LocalVersus && (!bLocalSecondJoined || bSecondDeviceMissing))
		{
			SelectStatus = bSecondDeviceMissing
				? TEXT("Player 2's controller disconnected. Reconnect it before the bout starts.")
				: TEXT("Waiting for player 2. Press Start on the second controller.");
			RebuildFighterSelect();
			return;
		}
		if (MatchSeat != 0 && SeatMode == EFBMatchSeatMode::Online)
		{
			SelectStatus = TEXT("Waiting for the host to start.");
			RebuildFighterSelect();
			return;
		}
		if (!bLeftLocked || !bRightLocked)
		{
			SelectStatus = TEXT("Both fighters have to be confirmed.");
			RebuildFighterSelect();
			return;
		}
		bBoutStartPending = true;
		ServerStartSeatedBout();
		return;
	}
	if (IsSequentialSelect() && (!bLeftLocked || !bRightLocked))
	{
		SelectStatus = bLeftLocked ? TEXT("Choose AI Opponent.") : TEXT("Choose Your Fighter.");
		RebuildFighterSelect();
		return;
	}
	const FFBFighterRecord& Chosen = bBuilderOpen ? Draft : SelectRoster[PlayerPick];
	const FFBFighterRecord& Opponent = SelectRoster[OpponentPick];
	if (!FFBFighterBalance::IsReady(Chosen.Ratings) || !FFBFighterBalance::IsReady(Opponent.Ratings))
	{
		SelectStatus = TEXT("Both fighters need a build that costs exactly 600.");
		RebuildFighterSelect();
		return;
	}
	if (FFBFighterBalance::ClassForDivision(Chosen.Division) != FFBFighterBalance::ClassForDivision(Opponent.Division))
	{
		SelectStatus = TEXT("Competitive fights stay inside one weight class.");
		RebuildFighterSelect();
		return;
	}
	if (bCareerSelect && !Opponent.bPremade)
	{
		SelectStatus = TEXT("Career fights a premade in your class.");
		RebuildFighterSelect();
		return;
	}
	if (bBuilderOpen && (Chosen.Id.IsEmpty() || Chosen.Id.StartsWith(TEXT("premade_"))))
	{
		SaveCurrentDraft();
		if (!FFBFighterBalance::IsReady(Draft.Ratings))
		{
			RebuildFighterSelect();
			return;
		}
	}
	const FFBFighterRecord ReadyPlayer = bBuilderOpen ? Draft : SelectRoster[PlayerPick];
	bBoutStartPending = true;
	SelectStatus = TEXT("Starting the bout.");
	RebuildFighterSelect();
	ServerStartCompetitiveBout(ReadyPlayer, Opponent, bCareerSelect ? EFBPlayMode::Career : EFBPlayMode::VersusAI, SelectDifficulty);
}

void AFBSparringPlayerController::ServerStartCompetitiveBout_Implementation(FFBFighterRecord PlayerBuild, FFBFighterRecord OpponentBuild, EFBPlayMode RequestedMode, EFBAIDifficulty Difficulty)
{
	if (!HasAuthority())
	{
		ClientBoutRefused(TEXT("The bout could not start."));
		return;
	}
	PlayerBuild.WeightClass = FFBFighterBalance::ClassForDivision(PlayerBuild.Division);
	OpponentBuild.WeightClass = FFBFighterBalance::ClassForDivision(OpponentBuild.Division);
	PlayerBuild.RulesetId = TEXT("CompetitiveFair");
	OpponentBuild.RulesetId = TEXT("CompetitiveFair");
	PlayerBuild.RulesetVersion = FFBFighterBalance::RulesetVersion;
	OpponentBuild.RulesetVersion = FFBFighterBalance::RulesetVersion;
	if (!FFBFighterBalance::IsReady(PlayerBuild.Ratings) || !FFBFighterBalance::IsReady(OpponentBuild.Ratings))
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused a bout: a build is not exactly 600."));
		ClientBoutRefused(TEXT("Both fighters need a build that costs exactly 600."));
		return;
	}
	if (PlayerBuild.WeightClass != OpponentBuild.WeightClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused a bout: different competitive classes."));
		ClientBoutRefused(TEXT("Competitive fights stay inside one weight class."));
		return;
	}
	const bool bCareer = RequestedMode == EFBPlayMode::Career;
	if (RequestedMode != EFBPlayMode::VersusAI && !bCareer)
	{
		ClientBoutRefused(TEXT("The bout could not start."));
		return;
	}
	if (bCareer)
	{
		TArray<FFBFighterRecord> Premades;
		FFBFighterBalance::GetPremades(Premades);
		const FFBFighterRecord* Found = nullptr;
		for (const FFBFighterRecord& Premade : Premades)
		{
			if (Premade.Id == OpponentBuild.Id)
			{
				Found = &Premade;
				break;
			}
		}
		if (!Found || Found->WeightClass != PlayerBuild.WeightClass || !FFBFighterBalance::IsReady(Found->Ratings))
		{
			UE_LOG(LogTemp, Warning, TEXT("FinalBell refused a career bout: opponent is not a premade in that class."));
			ClientBoutRefused(TEXT("Career fights a premade in your class."));
			return;
		}
		OpponentBuild = *Found;
	}
	if (RequestedMode == EFBPlayMode::Multiplayer)
	{
		ClientBoutRefused(TEXT("Use the seated start for a two-player bout."));
		return;
	}
	const EFBPlayMode StartMode = bCareer ? EFBPlayMode::Career : EFBPlayMode::VersusAI;
	Difficulty = FFBTactical::SanitizeDifficulty(Difficulty);
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		FrontEnd->SetPendingBout(PlayerBuild, OpponentBuild, Difficulty);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ChoosePlayMode(StartMode);
		}
	}
	AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr;
	if (!Mode)
	{
		ClientBoutRefused(TEXT("The bout could not start."));
		return;
	}
	Mode->NoteBoutRequester(this);
	Mode->SetAiDifficulty(Difficulty);
	Mode->SetAwaitingFighterConfirm(false);
	Mode->ApplyPlayMode(StartMode);
	FString Why;
	if (Mode->HadRosterRefusal(Why))
	{
		if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
		{
			FrontEnd->ClearPendingBout();
		}
		Mode->SetAwaitingFighterConfirm(true);
		ClientBoutRefused(Why);
		return;
	}
	BroadcastSoloPresentation();
	ClientBoutAccepted();
}

void AFBSparringPlayerController::ClientBoutAccepted_Implementation()
{
	bBoutStartPending = false;
	SelectStatus.Reset();
	HidePauseMenu();
	PresentSoloRing();
}

void AFBSparringPlayerController::ClientBoutRefused_Implementation(const FString& Why)
{
	bBoutStartPending = false;
	SelectStatus = Why.IsEmpty() ? TEXT("The bout was refused.") : Why;
	if (!bFighterSelectVisible)
	{
		ShowFighterSelect();
		return;
	}
	RebuildFighterSelect();
}

void AFBSparringPlayerController::ServerCommitFighter_Implementation(FFBFighterRecord Record)
{
	if (!HasAuthority() || MatchSeat < 0 || !GetWorld())
	{
		return;
	}
	AFBSparringGameState* State = GetWorld()->GetGameState<AFBSparringGameState>();
	if (!State)
	{
		return;
	}
	Record.WeightClass = FFBFighterBalance::ClassForDivision(Record.Division);
	Record.RulesetId = TEXT("CompetitiveFair");
	Record.RulesetVersion = FFBFighterBalance::RulesetVersion;
	if (MatchSeat == 0)
	{
		State->bBlueReady = false;
		State->BluePick = Record;
		State->bBlueReady = true;
	}
	else if (MatchSeat == 1)
	{
		State->bRedReady = false;
		State->RedPick = Record;
		State->bRedReady = true;
	}
	State->ForceNetUpdate();
}

void AFBSparringPlayerController::ServerClearSeat_Implementation()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}
	AFBSparringGameState* State = GetWorld()->GetGameState<AFBSparringGameState>();
	if (!State)
	{
		return;
	}
	if (MatchSeat == 0)
	{
		State->bBlueReady = false;
	}
	else if (MatchSeat == 1)
	{
		State->bRedReady = false;
	}
	State->ForceNetUpdate();
}

void AFBSparringPlayerController::ServerStartSeatedBout_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	if (MatchSeat != 0)
	{
		ClientBoutRefused(TEXT("Only player 1 can start."));
		return;
	}
	AFBSparringGameState* State = GetWorld() ? GetWorld()->GetGameState<AFBSparringGameState>() : nullptr;
	AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr;
	if (!State || !Mode)
	{
		ClientBoutRefused(TEXT("The bout could not start."));
		return;
	}
	if (State->LobbyProtocolSeen != AFBSparringGameState::LobbyProtocol)
	{
		ClientBoutRefused(TEXT("This match is a different build. Use the same packaged game."));
		return;
	}
	if (!State->bBlueReady || !State->bRedReady)
	{
		ClientBoutRefused(TEXT("Both fighters have to be confirmed."));
		return;
	}
	if (!FFBFighterBalance::IsReady(State->BluePick.Ratings) || !FFBFighterBalance::IsReady(State->RedPick.Ratings))
	{
		ClientBoutRefused(TEXT("Both fighters need a build that costs exactly 600."));
		return;
	}
	if (FFBFighterBalance::ClassForDivision(State->BluePick.Division) != FFBFighterBalance::ClassForDivision(State->RedPick.Division))
	{
		ClientBoutRefused(TEXT("Competitive fights stay inside one weight class."));
		return;
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		FrontEnd->SetPendingBout(State->BluePick, State->RedPick, SelectDifficulty);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ChoosePlayMode(EFBPlayMode::Multiplayer);
		}
	}
	Mode->NoteBoutRequester(this);
	Mode->SetAwaitingFighterConfirm(false);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Other = It->Get();
		if (Other && (Other->IsInState(NAME_Spectating) || Other->GetPawn() == nullptr))
		{
			Mode->RestartPlayer(Other);
		}
	}
	Mode->ApplyPlayMode(EFBPlayMode::Multiplayer);
	HideFighterSelect();
	ClientBoutAccepted();
}

bool AFBSparringPlayerController::IsLegalOpponentIndex(int32 Index) const
{
	if (!SelectRoster.IsValidIndex(Index))
	{
		return false;
	}
	const EFBWeightClassId Want = bBuilderOpen
		? FFBFighterBalance::ClassForDivision(Draft.Division)
		: (SelectRoster.IsValidIndex(PlayerPick) ? SelectRoster[PlayerPick].WeightClass : EFBWeightClassId::Middleweight);
	const FFBFighterRecord& Fighter = SelectRoster[Index];
	if (Fighter.WeightClass != Want || !FFBFighterBalance::IsReady(Fighter.Ratings))
	{
		return false;
	}
	if (bCareerSelect && !Fighter.bPremade)
	{
		return false;
	}
	return bBuilderOpen || Index != PlayerPick;
}

void AFBSparringPlayerController::CollectLegalOpponents(TArray<int32>& Out) const
{
	Out.Reset();
	for (int32 Index = 0; Index < SelectRoster.Num(); ++Index)
	{
		if (IsLegalOpponentIndex(Index))
		{
			Out.Add(Index);
		}
	}
}

void AFBSparringPlayerController::EnsureOpponentInClass()
{
	if (IsLegalOpponentIndex(OpponentPick))
	{
		return;
	}
	const EFBDivisionId WantDivision = bBuilderOpen
		? Draft.Division
		: (SelectRoster.IsValidIndex(PlayerPick) ? SelectRoster[PlayerPick].Division : EFBDivisionId::Middleweight);
	int32 SameDivision = INDEX_NONE;
	int32 Any = INDEX_NONE;
	for (int32 Index = 0; Index < SelectRoster.Num(); ++Index)
	{
		if (!IsLegalOpponentIndex(Index))
		{
			continue;
		}
		if (Any == INDEX_NONE)
		{
			Any = Index;
		}
		if (SameDivision == INDEX_NONE && SelectRoster[Index].Division == WantDivision)
		{
			SameDivision = Index;
		}
	}
	if (SameDivision != INDEX_NONE)
	{
		OpponentPick = SameDivision;
	}
	else if (Any != INDEX_NONE)
	{
		OpponentPick = Any;
	}
}

void AFBSparringPlayerController::CycleOpponent()
{
	TArray<int32> Legal;
	CollectLegalOpponents(Legal);
	if (Legal.Num() == 0)
	{
		return;
	}
	int32 Slot = Legal.IndexOfByKey(OpponentPick);
	Slot = Slot == INDEX_NONE ? 0 : (Slot + 1) % Legal.Num();
	OpponentPick = Legal[Slot];
}

void AFBSparringPlayerController::RebuildFighterSelect()
{
	SelectRows.Reset();
	SelectActions.Reset();
	CreatorMeasure.Reset();
	if (!GEngine || !LocalViewport())
	{
		return;
	}
	if (FighterSelectWidget.IsValid())
	{
		LocalViewport()->RemoveViewportWidgetContent(FighterSelectWidget.ToSharedRef());
		FighterSelectWidget.Reset();
	}

	EnsureOpponentInClass();
	const FLinearColor Paper = FBMenuArt::Paper();
	const FLinearColor Accent = FBMenuArt::Accent();
	const FLinearColor Quiet = FBMenuArt::Quiet();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

	const int32 Spent = FFBFighterBalance::CostOfBuild(Draft.Ratings);
	const FString Budget = FString::Printf(TEXT("Spent %d / 600   %s"), Spent < 0 ? 0 : Spent, *SelectStatus);

	TArray<int32> FocusSides;
	auto StampSide = [&](int32 Side)
	{
		while (FocusSides.Num() < SelectRows.Num())
		{
			FocusSides.Add(Side);
		}
	};
	auto AddPickRow = [&](TSharedRef<SVerticalBox> List, int32 Index, bool bChosen, TFunction<void()> OnPick)
	{
		const FFBFighterRecord& Fighter = SelectRoster[Index];
		const FString Line = FString::Printf(TEXT("%s   ·   %s"),
			*Fighter.DisplayName.ToUpper(),
			*StyleLabel(Fighter).ToUpper());
		TSharedPtr<SBorder> Fill;
		TSharedPtr<SBorder> Tick;
		TSharedPtr<STextBlock> NameText;
		SAssignNew(Fill, SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(FMargin(0.f, 3.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox).WidthOverride(4.f).HeightOverride(16.f)
					[
						SNew(SBorder)
						.BorderImage(White)
						.BorderBackgroundColor(bChosen ? Accent : FLinearColor::Transparent)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.f, 2.f, 8.f, 2.f)
				[
					SNew(SBox).WidthOverride(4.f)
					[
						SAssignNew(Tick, SBorder)
						.BorderImage(White)
						.BorderBackgroundColor(FLinearColor::Transparent)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SAssignNew(NameText, STextBlock)
					.Text(FText::FromString(Line))
					.Font(FBMenuArt::PosterFont(13, 12))
					.ColorAndOpacity(bChosen ? Paper : Quiet)
				]
			];
		const int32 FocusIndex = SelectRows.Num();
		FFBRowChrome Chrome;
		Chrome.Fill = Fill;
		Chrome.Tick = Tick;
		Chrome.Label = NameText;
		SelectRows.Add(Chrome);
		SelectActions.Add([this, OnPick, FocusIndex]()
		{
			SelectFocus = FocusIndex;
			OnPick();
		});
		List->AddSlot().AutoHeight().Padding(0.f, 1.f)
		[
			SNew(SButton)
			.ButtonStyle(&ClearButton())
			.OnHovered_Lambda([this, FocusIndex]()
			{
				const int32 Side = SelectSides.IsValidIndex(FocusIndex) ? SelectSides[FocusIndex] : 0;
				if (!CanTouchSelectSide(Side, true))
				{
					return;
				}
				RememberFocus(Side, FocusIndex);
				PaintSelectChrome();
			})
			.OnClicked_Lambda([this, OnPick, FocusIndex]()
			{
				const int32 Side = SelectSides.IsValidIndex(FocusIndex) ? SelectSides[FocusIndex] : 0;
				if (!CanTouchSelectSide(Side, true))
				{
					return FReply::Handled();
				}
				RememberFocus(Side, FocusIndex);
				OnPick();
				return FReply::Handled();
			})
			[
				Fill.ToSharedRef()
			]
		];
	};

	TSharedRef<SVerticalBox> PlayerList = SNew(SVerticalBox);
	for (int32 Index = 0; Index < SelectRoster.Num(); ++Index)
	{
		AddPickRow(PlayerList, Index, !bBuilderOpen && Index == PlayerPick, [this, Index]()
		{
			if (!CanTouchSelectSide(0, MenuControllerId == 0 && bShowMouseCursor))
			{
				if (!CanTouchSelectSide(0, false))
				{
					return;
				}
			}
			if (bLeftLocked && IsSequentialSelect())
			{
				return;
			}
			PlayerPick = Index;
			bBuilderOpen = false;
			if (SelectRoster.IsValidIndex(Index))
			{
				Draft = SelectRoster[Index];
				if (UsesSplitSeats() || IsSequentialSelect())
				{
					bLeftLocked = true;
					ServerCommitFighter(SelectRoster[Index]);
				}
			}
			if (IsSequentialSelect())
			{
				ActiveSide = 1;
				SelectStatus = TEXT("Choose AI Opponent.");
			}
			else
			{
				SelectStatus = TEXT("Player 1 confirmed.");
			}
			RebuildFighterSelect();
		});
	}
	StampSide(0);

	TSharedRef<SVerticalBox> Builder = SNew(SVerticalBox);
	if (bBuilderOpen)
	{
		Builder->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Draft.DisplayName))
			.OnTextChanged_Lambda([this](const FText& Text) { Draft.DisplayName = Text.ToString(); })
		];
		auto AddChoice = [&](const FString& Label, TFunction<void()> Step)
		{
			Builder->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				MakeFocusRow(Label, [this, Step]()
				{
					Step();
					RebuildFighterSelect();
				})
			];
		};
		auto Step = [](float& Value)
		{
			Value = Value >= 0.85f ? 0.05f : FMath::Clamp(Value + 0.15f, 0.f, 1.f);
		};
		auto PushLook = [this]()
		{
			if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
			{
				Boxer->PreviewCreatorLook(Draft.Look);
			}
			if (CreatorMeasure.IsValid())
			{
				CreatorMeasure->SetText(FText::FromString(GetCreatorMeasureText()));
			}
		};
		auto AddSlider = [&](const FString& Name, float FFBPresentation::*Field)
		{
			AddChoice(FString::Printf(TEXT("%s %.0f"), *Name, Draft.Look.*Field * 100.f), [this, Field]()
			{
				float& Value = Draft.Look.*Field;
				Value = Value >= 0.90f ? 0.05f : FMath::Clamp(Value + 0.1f, 0.f, 1.f);
			});
			Builder->AddSlot().AutoHeight().Padding(10.f, 0.f, 4.f, 6.f)
			[
				SNew(SSlider)
				.Value(Draft.Look.*Field)
				.OnValueChanged_Lambda([this, Field, PushLook](float New)
				{
					Draft.Look.*Field = FMath::Clamp(New, 0.f, 1.f);
					PushLook();
				})
			];
		};
		const TCHAR* Pages[] = { TEXT("Identity"), TEXT("Body"), TEXT("Face"), TEXT("Skin"), TEXT("Hair"), TEXT("Ratings") };
		for (int32 Page = 0; Page < 6; ++Page)
		{
			const FString Label = (Page == BuilderPage ? TEXT("* ") : TEXT("")) + FString(Pages[Page]);
			AddChoice(Label, [this, Page]() { BuilderPage = Page; });
		}
		AddChoice(TEXT("Randomize look"), [this]()
		{
			Draft.Stance = FMath::RandBool() ? EFBStance::Southpaw : EFBStance::Orthodox;
			FFBBoxerCustomization::Randomize(Draft.Look, FMath::Rand());
			SelectStatus = TEXT("Look randomized. Ratings stay on the 600-point budget.");
		});
		AddChoice(TEXT("Reset look"), [this]()
		{
			const FString Nickname = Draft.Look.Nickname;
			const FString Country = Draft.Look.Country;
			FFBBoxerCustomization::ResetLook(Draft.Look);
			Draft.Look.Nickname = Nickname;
			Draft.Look.Country = Country;
			SelectStatus = TEXT("Look reset. Ratings stay on the 600-point budget.");
		});
		AddChoice(TEXT("Zoom in"), [this]() { AddCreatorZoom(-0.12f); });
		AddChoice(TEXT("Zoom out"), [this]() { AddCreatorZoom(0.12f); });
		AddChoice(TEXT("Back to roster"), [this]()
		{
			bBuilderOpen = false;
			SelectStatus.Reset();
		});
		TArray<FFBFighterRecord> Starters;
		FFBFighterBalance::GetPremades(Starters);
		for (const FFBFighterRecord& Starter : Starters)
		{
			AddChoice(FString::Printf(TEXT("Start from %s"), *Starter.DisplayName), [this, Starter]()
			{
				const FString KeepId = Draft.Id;
				const bool Fought = Draft.bFoughtOnce;
				const int32 Bank = Draft.RetrainBudget;
				Draft.Ratings = Starter.Ratings;
				Draft.Look = Starter.Look;
				Draft.Division = Starter.Division;
				Draft.WeightClass = Starter.WeightClass;
				Draft.Stance = Starter.Stance;
				Draft.Plan = Starter.Plan;
				Draft.Style = Starter.Style;
				Draft.Id = KeepId;
				Draft.bPremade = false;
				Draft.bFoughtOnce = Fought;
				Draft.RetrainBudget = Bank;
				if (Draft.DisplayName.IsEmpty() || Draft.DisplayName == TEXT("Your Fighter"))
				{
					Draft.DisplayName = Starter.DisplayName;
				}
				Draft.Look.MetaHumanAsset = TEXT("/Game/MetaHumans/MH_FB_CustomTemplate");
			});
		}
		if (BuilderPage == 0)
		{
			Builder->AddSlot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SEditableTextBox)
				.Text(FText::FromString(Draft.Look.Nickname))
				.OnTextChanged_Lambda([this](const FText& Text) { Draft.Look.Nickname = Text.ToString(); })
			];
			AddChoice(FString::Printf(TEXT("Division: %s"), *FFBFighterBalance::DivisionName(Draft.Division)), [this]()
			{
				Draft.Division = static_cast<EFBDivisionId>((static_cast<uint8>(Draft.Division) + 1) % 8);
				Draft.WeightClass = FFBFighterBalance::ClassForDivision(Draft.Division);
			});
			AddChoice(Draft.Stance == EFBStance::Southpaw ? TEXT("Stance: Southpaw") : TEXT("Stance: Orthodox"), [this]()
			{
				Draft.Stance = Draft.Stance == EFBStance::Orthodox ? EFBStance::Southpaw : EFBStance::Orthodox;
			});
			Builder->AddSlot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SEditableTextBox)
				.HintText(FText::FromString(TEXT("Hometown")))
				.Text(FText::FromString(Draft.Look.Hometown))
				.OnTextChanged_Lambda([this](const FText& Text) { Draft.Look.Hometown = Text.ToString().Left(FFBBoxerCustomization::HometownMax); })
			];
			AddChoice(FString::Printf(TEXT("Age: %d"), Draft.Look.Age), [this]()
			{
				Draft.Look.Age = Draft.Look.Age >= FFBBoxerCustomization::AgeMax ? FFBBoxerCustomization::AgeMin : Draft.Look.Age + 1;
			});
			AddChoice(FString::Printf(TEXT("Country: %s"), *Draft.Look.Country), [this]()
			{
				const TCHAR* Countries[] = { TEXT("USA"), TEXT("Mexico"), TEXT("Cuba"), TEXT("UK"), TEXT("Ukraine"), TEXT("Nigeria"), TEXT("Japan"), TEXT("Philippines") };
				int32 Found = 0;
				for (int32 Index = 0; Index < 8; ++Index)
				{
					if (Draft.Look.Country == Countries[Index]) { Found = Index; break; }
				}
				Draft.Look.Country = Countries[(Found + 1) % 8];
			});
			AddChoice(Draft.Look.SexPresentation <= 0 ? TEXT("Masculine / feminine: feminine") : (Draft.Look.SexPresentation == 1 ? TEXT("Masculine / feminine: in between") : TEXT("Masculine / feminine: masculine")), [this]()
			{
				Draft.Look.SexPresentation = (Draft.Look.SexPresentation + 1) % 3;
				Draft.Look.Masculine = Draft.Look.SexPresentation / 2.f;
			});
			const TCHAR* Colors[] = { TEXT("Red"), TEXT("Blue"), TEXT("Black"), TEXT("Gold"), TEXT("White"), TEXT("Green") };
			AddChoice(FString::Printf(TEXT("Gloves: %s"), Colors[Draft.Look.GloveColor % 6]), [this]() { Draft.Look.GloveColor = (Draft.Look.GloveColor + 1) % 6; });
			AddChoice(FString::Printf(TEXT("Trunks: %s"), Colors[Draft.Look.TrunksColor % 6]), [this]() { Draft.Look.TrunksColor = (Draft.Look.TrunksColor + 1) % 6; });
			AddChoice(FString::Printf(TEXT("Stripe: %s"), Colors[Draft.Look.TrunksStripe % 6]), [this]() { Draft.Look.TrunksStripe = (Draft.Look.TrunksStripe + 1) % 6; });
			AddChoice(FString::Printf(TEXT("Shoes: %s"), Colors[Draft.Look.ShoeColor % 6]), [this]() { Draft.Look.ShoeColor = (Draft.Look.ShoeColor + 1) % 6; });
			AddChoice(FString::Printf(TEXT("Hand wraps: %s"), Colors[Draft.Look.WrapColor % 6]), [this]() { Draft.Look.WrapColor = (Draft.Look.WrapColor + 1) % 6; });
			AddChoice(FString::Printf(TEXT("Glove style: %s"), FFBBoxerCustomization::GloveStyleName(Draft.Look.GloveStyle)), [this]() { Draft.Look.GloveStyle = (Draft.Look.GloveStyle + 1) % 3; });
			AddChoice(FString::Printf(TEXT("Trunks style: %s"), FFBBoxerCustomization::TrunksStyleName(Draft.Look.TrunksStyle)), [this]() { Draft.Look.TrunksStyle = (Draft.Look.TrunksStyle + 1) % 3; });
			AddChoice(FString::Printf(TEXT("Shoe style: %s"), FFBBoxerCustomization::ShoeStyleName(Draft.Look.ShoeStyle)), [this]() { Draft.Look.ShoeStyle = (Draft.Look.ShoeStyle + 1) % 2; });
			AddChoice(FString::Printf(TEXT("Wrap style: %s"), FFBBoxerCustomization::WrapStyleName(Draft.Look.WrapStyle)), [this]() { Draft.Look.WrapStyle = (Draft.Look.WrapStyle + 1) % 3; });
			AddChoice(FString::Printf(TEXT("Accessory: %s"), FFBBoxerCustomization::AccessoryName(Draft.Look.Accessory)), [this]() { Draft.Look.Accessory = (Draft.Look.Accessory + 1) % 4; });
		}
		else if (BuilderPage == 1)
		{
			AddSlider(TEXT("Height"), &FFBPresentation::Height);
			AddSlider(TEXT("Reach"), &FFBPresentation::Reach);
			AddSlider(TEXT("Shoulders"), &FFBPresentation::ShoulderWidth);
			AddSlider(TEXT("Chest"), &FFBPresentation::Chest);
			AddSlider(TEXT("Torso"), &FFBPresentation::TorsoLength);
			AddSlider(TEXT("Arms"), &FFBPresentation::ArmLength);
			AddSlider(TEXT("Legs"), &FFBPresentation::LegLength);
			AddSlider(TEXT("Muscle"), &FFBPresentation::Muscularity);
			AddSlider(TEXT("Fat"), &FFBPresentation::BodyFat);
			AddSlider(TEXT("Neck"), &FFBPresentation::Neck);
			AddSlider(TEXT("Waist"), &FFBPresentation::Waist);
			AddSlider(TEXT("Hip"), &FFBPresentation::Hip);
			AddSlider(TEXT("Bicep"), &FFBPresentation::Bicep);
			AddSlider(TEXT("Thigh"), &FFBPresentation::Thigh);
		}
		else if (BuilderPage == 2)
		{
			AddSlider(TEXT("Head width"), &FFBPresentation::HeadWidth);
			AddSlider(TEXT("Head length"), &FFBPresentation::HeadLength);
			AddSlider(TEXT("Brow"), &FFBPresentation::Brow);
			AddSlider(TEXT("Eyes"), &FFBPresentation::Eyes);
			AddSlider(TEXT("Nose"), &FFBPresentation::Nose);
			AddSlider(TEXT("Cheeks"), &FFBPresentation::Cheeks);
			AddSlider(TEXT("Jaw"), &FFBPresentation::Jaw);
			AddSlider(TEXT("Chin"), &FFBPresentation::Chin);
			AddSlider(TEXT("Lips"), &FFBPresentation::Lips);
			AddSlider(TEXT("Ears"), &FFBPresentation::Ears);
			AddSlider(TEXT("Asymmetry"), &FFBPresentation::Asymmetry);
		}
		else if (BuilderPage == 3)
		{
			AddChoice(FString::Printf(TEXT("Skin U %.0f"), Draft.Look.SkinU * 100.f), [this, Step]() { Step(Draft.Look.SkinU); });
			AddChoice(FString::Printf(TEXT("Skin V %.0f"), Draft.Look.SkinV * 100.f), [this, Step]() { Step(Draft.Look.SkinV); });
			const TCHAR* Freckles[] = { TEXT("None"), TEXT("Type 1"), TEXT("Type 2"), TEXT("Type 3") };
			AddChoice(FString::Printf(TEXT("Freckles: %s"), Freckles[FMath::Clamp(Draft.Look.FreckleMask, 0, 3)]), [this]() { Draft.Look.FreckleMask = (Draft.Look.FreckleMask + 1) % 4; });
			AddChoice(FString::Printf(TEXT("Freckle density %.0f"), Draft.Look.FreckleDensity * 100.f), [this, Step]() { Step(Draft.Look.FreckleDensity); });
			AddChoice(FString::Printf(TEXT("Iris %d"), Draft.Look.IrisPattern + 1), [this]() { Draft.Look.IrisPattern = (Draft.Look.IrisPattern + 1) % 9; });
			AddChoice(FString::Printf(TEXT("Iris hue %.0f"), Draft.Look.IrisHue * 100.f), [this, Step]() { Step(Draft.Look.IrisHue); });
			AddChoice(FString::Printf(TEXT("Foundation %.0f"), Draft.Look.Foundation * 100.f), [this, Step]() { Step(Draft.Look.Foundation); });
			AddChoice(FString::Printf(TEXT("Blush %.0f"), Draft.Look.Blush * 100.f), [this, Step]() { Step(Draft.Look.Blush); });
			AddChoice(FString::Printf(TEXT("Lip makeup %.0f"), Draft.Look.LipMakeup * 100.f), [this, Step]() { Step(Draft.Look.LipMakeup); });
			AddChoice(FString::Printf(TEXT("Tooth length %.0f"), (Draft.Look.ToothLength + 1.f) * 50.f), [this]()
			{
				Draft.Look.ToothLength = Draft.Look.ToothLength >= 0.8f ? -1.f : Draft.Look.ToothLength + 0.4f;
			});
		}
		else if (BuilderPage == 4)
		{
			const TCHAR* HairNames[] = { TEXT("Bald"), TEXT("Crop"), TEXT("Short"), TEXT("Fade"), TEXT("Braids") };
			AddChoice(FString::Printf(TEXT("Hair: %s"), HairNames[FMath::Clamp(static_cast<int32>(Draft.Look.Hair), 0, 4)]), [this]()
			{
				Draft.Look.Hair = static_cast<EFBHairStyle>((static_cast<uint8>(Draft.Look.Hair) + 1) % 5);
			});
			AddChoice(FString::Printf(TEXT("Gray %.0f"), Draft.Look.HairGray * 100.f), [this, Step]() { Step(Draft.Look.HairGray); });
			const TCHAR* Beards[] = { TEXT("None"), TEXT("Stubble"), TEXT("Short"), TEXT("Full") };
			AddChoice(FString::Printf(TEXT("Beard: %s"), Beards[FMath::Clamp(Draft.Look.Beard, 0, 3)]), [this]() { Draft.Look.Beard = (Draft.Look.Beard + 1) % 4; });
			AddChoice(Draft.Look.Mustache > 0 ? TEXT("Mustache: on") : TEXT("Mustache: off"), [this]() { Draft.Look.Mustache = Draft.Look.Mustache > 0 ? 0 : 1; });
			const TCHAR* Lashes[] = { TEXT("None"), TEXT("Short sparse"), TEXT("Short fine"), TEXT("Short thin"), TEXT("Long slight curl"), TEXT("Long curl"), TEXT("Long thick curl") };
			AddChoice(FString::Printf(TEXT("Eyelashes: %s"), Lashes[FMath::Clamp(Draft.Look.EyelashType, 0, 6)]), [this]() { Draft.Look.EyelashType = (Draft.Look.EyelashType + 1) % 7; });
			const TCHAR* Places[] = { TEXT("None"), TEXT("Arms"), TEXT("Chest"), TEXT("Back") };
			AddChoice(FString::Printf(TEXT("Tattoo place: %s"), Places[FMath::Clamp(Draft.Look.TattooPlace, 0, 3)]), [this]() { Draft.Look.TattooPlace = (Draft.Look.TattooPlace + 1) % 4; });
			AddChoice(FString::Printf(TEXT("Tattoo density %.0f"), Draft.Look.Tattoos * 100.f), [this, Step]() { Step(Draft.Look.Tattoos); });
			const TCHAR* Scars[] = { TEXT("None"), TEXT("Brow"), TEXT("Cheek"), TEXT("Nose") };
			AddChoice(FString::Printf(TEXT("Scar: %s"), Scars[FMath::Clamp(Draft.Look.Scar, 0, 3)]), [this]() { Draft.Look.Scar = (Draft.Look.Scar + 1) % 4; });
			AddChoice(FString::Printf(TEXT("Groom slot: %s"), *Draft.Look.GroomSlot), [this]()
			{
				const TCHAR* Grooms[] = { TEXT("Hair"), TEXT("Eyebrows"), TEXT("Beard"), TEXT("Mustache"), TEXT("Eyelashes"), TEXT("Peachfuzz") };
				int32 Found = 0;
				for (int32 Index = 0; Index < 6; ++Index)
				{
					if (Draft.Look.GroomSlot == Grooms[Index]) { Found = Index; break; }
				}
				Draft.Look.GroomSlot = Grooms[(Found + 1) % 6];
			});
		}
		if (Draft.Ratings.Num() != FFBFighterBalance::AttributeCount)
		{
			Draft.Ratings = FFBFighterBalance::PresetBalanced();
		}
		if (BuilderPage == 5)
		for (int32 Attr = 0; Attr < FFBFighterBalance::AttributeCount; ++Attr)
		{
			const EFBAttributeId Id = static_cast<EFBAttributeId>(Attr);
			const int32 Rating = Draft.Ratings[Attr];
			const FString Row = FString::Printf(TEXT("%s  %d   next %d"),
				*FFBFighterBalance::AttributeName(Id), Rating, FFBFighterBalance::NextStepCost(Rating));
			Builder->AddSlot().AutoHeight().Padding(0.f, 1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeFocusRow(FString::Printf(TEXT("Lower %s"), *Row), [this, Attr]()
					{
						if (Draft.Ratings.IsValidIndex(Attr) && Draft.Ratings[Attr] > FFBFighterBalance::RatingMin)
						{
							--Draft.Ratings[Attr];
							Draft.Style = TEXT("Custom");
						}
						RebuildFighterSelect();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					MakeFocusRow(FString::Printf(TEXT("Raise %s"), *Row), [this, Attr]()
					{
						if (!Draft.Ratings.IsValidIndex(Attr) || Draft.Ratings[Attr] >= FFBFighterBalance::RatingMax)
						{
							return;
						}
						TArray<int32> Trial = Draft.Ratings;
						++Trial[Attr];
						FString Why;
						FFBFighterRecord Gate = Draft;
						Gate.Ratings = Draft.Ratings;
						if (FFBFighterBalance::CanRetireInto(Gate, Trial, Why) && FFBFighterBalance::IsDraftLegal(Trial))
						{
							Draft.Ratings = Trial;
							Draft.Style = TEXT("Custom");
						}
						else
						{
							SelectStatus = Why.IsEmpty() ? TEXT("That point would put the build over 600.") : Why;
						}
						RebuildFighterSelect();
					})
				]
			];
		}
		auto PresetButton = [&](const FString& Name, TArray<int32> Ratings)
		{
			Builder->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				MakeFocusRow(Name, [this, Ratings, Name]()
				{
					Draft.Ratings = Ratings;
					Draft.Plan = TEXT("Custom");
					Draft.Style = Name;
					RebuildFighterSelect();
				})
			];
		};
		PresetButton(TEXT("Balanced"), FFBFighterBalance::PresetBalanced());
		PresetButton(TEXT("Puncher"), FFBFighterBalance::PresetPuncher());
		PresetButton(TEXT("Counter"), FFBFighterBalance::PresetCounter());
		PresetButton(TEXT("Pressure"), FFBFighterBalance::PresetPressure());
	}

	StampSide(1);

	auto ActionRow = [&](const FString& Label, TFunction<FReply()> Click) -> TSharedRef<SWidget>
	{
		return MakeFocusRow(Label, [Click]()
		{
			Click();
		});
	};

	const FFBFighterRecord PlayerCard = bBuilderOpen
		? Draft
		: (SelectRoster.IsValidIndex(PlayerPick) ? SelectRoster[PlayerPick] : FFBFighterRecord());
	const bool bOpponentReady = !bBuilderOpen && SelectRoster.IsValidIndex(OpponentPick) && IsLegalOpponentIndex(OpponentPick);
	const FFBFighterRecord OpponentCard = bOpponentReady ? SelectRoster[OpponentPick] : FFBFighterRecord();
	const EFBWeightClassId WantedClass = bBuilderOpen
		? FFBFighterBalance::ClassForDivision(Draft.Division)
		: PlayerCard.WeightClass;

	TSharedPtr<SWidget> SaveRow;
	TSharedRef<SVerticalBox> OpponentList = SNew(SVerticalBox);
	if (bBuilderOpen)
	{
		SaveRow = ActionRow(TEXT("SAVE"), [this]()
		{
			SaveCurrentDraft();
			RebuildFighterSelect();
			return FReply::Handled();
		});
		StampSide(1);
	}
	else
	{
		TArray<int32> LegalOpponents;
		CollectLegalOpponents(LegalOpponents);
		if (LegalOpponents.Num() == 0)
		{
			OpponentList->AddSlot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("None in this class")))
				.Font(FBMenuArt::PosterFont(16, 20))
				.ColorAndOpacity(Paper)
			];
		}
		const bool bWaitingForLocalTwo = SeatMode == EFBMatchSeatMode::LocalVersus && !bLocalSecondJoined;
		for (int32 Index : LegalOpponents)
		{
			if (bWaitingForLocalTwo)
			{
				break;
			}
			AddPickRow(OpponentList, Index, Index == OpponentPick, [this, Index]()
			{
				if (!CanTouchSelectSide(1, false) && !CanTouchSelectSide(1, true))
				{
					return;
				}
				if (IsSequentialSelect() && !bLeftLocked)
				{
					SelectStatus = TEXT("Choose Your Fighter.");
					RebuildFighterSelect();
					return;
				}
				OpponentPick = Index;
				if (SelectRoster.IsValidIndex(Index) && (UsesSplitSeats() || IsSequentialSelect()))
				{
					bRightLocked = true;
					if (UsesSplitSeats())
					{
						ServerCommitFighter(SelectRoster[Index]);
					}
				}
				if (IsSequentialSelect())
				{
					ActiveSide = 2;
					SelectStatus = TEXT("Both fighters are set.");
				}
				else
				{
					SelectStatus = SeatMode == EFBMatchSeatMode::Online ? TEXT("Player 2 confirmed.") : TEXT("Player 2 confirmed.");
				}
				RebuildFighterSelect();
			});
		}
		OpponentList->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			ActionRow(TEXT("NEXT OPPONENT"), [this]()
			{
				CycleOpponent();
				SelectStatus.Reset();
				RebuildFighterSelect();
				return FReply::Handled();
			})
		];
		StampSide(1);
	}

	TSharedRef<SWidget> FightRow = ActionRow(TEXT("FIGHT"), [this]()
	{
		ConfirmCompetitiveBout();
		return FReply::Handled();
	});
	TSharedRef<SWidget> BackRow = ActionRow(TEXT("BACK"), [this]()
	{
		HideFighterSelect();
		ShowModeMenu();
		return FReply::Handled();
	});
	TSharedRef<SWidget> CreateRow = ActionRow(TEXT("CREATE A FIGHTER"), [this]()
	{
		bBuilderOpen = true;
		Draft = FFBFighterRecord();
		Draft.DisplayName = TEXT("Your Fighter");
		Draft.Ratings = FFBFighterBalance::PresetBalanced();
		Draft.Plan = TEXT("Custom");
		Draft.Style = TEXT("Custom");
		Draft.RulesetId = TEXT("CompetitiveFair");
		Draft.RulesetVersion = FFBFighterBalance::RulesetVersion;
		Draft.Look.MetaHumanAsset = TEXT("/Game/MetaHumans/MH_FB_CustomTemplate");
		Draft.Look.SkinU = 0.5f;
		Draft.Look.SkinV = 0.5f;
		BuilderPage = 0;
		SelectStatus = TEXT("New fighter. Save it, then fight. Ten is the limit.");
		SelectFocus = SelectRoster.Num();
		RebuildFighterSelect();
		return FReply::Handled();
	});
	const TCHAR* DifficultyName = TEXT("CLUB");
	if (SelectDifficulty == EFBAIDifficulty::Novice)
	{
		DifficultyName = TEXT("NOVICE");
	}
	else if (SelectDifficulty == EFBAIDifficulty::Expert)
	{
		DifficultyName = TEXT("EXPERT");
	}
	TSharedRef<SWidget> DifficultyRow = ActionRow(FString::Printf(TEXT("AI: %s"), DifficultyName), [this]()
	{
		const uint8 Next = static_cast<uint8>(SelectDifficulty) + 1;
		SelectDifficulty = Next > static_cast<uint8>(EFBAIDifficulty::Expert)
			? EFBAIDifficulty::Novice
			: static_cast<EFBAIDifficulty>(Next);
		if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
		{
			Saved->AIDifficulty = SelectDifficulty;
			Saved->WriteToDisk();
		}
		RebuildFighterSelect();
		return FReply::Handled();
	});
	StampSide(2);

	auto AddIdentity = [&](TSharedRef<SVerticalBox> Body, const FFBFighterRecord& Fighter)
	{
		const bool bHas = !Fighter.DisplayName.IsEmpty() && Fighter.Ratings.Num() == FFBFighterBalance::AttributeCount;
		if (!bHas)
		{
			Body->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("None in this class")))
				.Font(FBMenuArt::PosterFont(22, 30))
				.ColorAndOpacity(Paper)
			];
			return;
		}
		const FString StanceName = Fighter.Stance == EFBStance::Southpaw ? TEXT("Southpaw") : TEXT("Orthodox");
		Body->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
			[
				PortraitStandIn(Fighter)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(StyleLabel(Fighter).ToUpper()))
					.Font(FBMenuArt::PosterFont(13, 40))
					.ColorAndOpacity(Accent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Fighter.DisplayName.ToUpper()))
					.Font(FBMenuArt::PosterFont(26, 20))
					.ColorAndOpacity(Paper)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%s   ·   %s"),
						*FFBFighterBalance::DivisionName(Fighter.Division).ToUpper(),
						*StanceName.ToUpper())))
					.Font(FBMenuArt::PosterFont(12, 16))
					.ColorAndOpacity(Quiet)
					.AutoWrapText(true)
				]
			]
		];
		Body->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("RATINGS")))
			.Font(FBMenuArt::PosterFont(11, 50))
			.ColorAndOpacity(Accent)
		];
		Body->AddSlot().AutoHeight()
		[
			RatingGrid(Fighter, Paper, Accent, Quiet)
		];
	};

	TSharedPtr<STextBlock> LeftTitle;
	TSharedPtr<STextBlock> RightTitle;
	TSharedRef<SVerticalBox> LeftBody = SNew(SVerticalBox);
	LeftBody->AddSlot().AutoHeight()
	[
		SAssignNew(LeftTitle, STextBlock)
		.Text(FText::FromString(SeatMode == EFBMatchSeatMode::LocalVersus || SeatMode == EFBMatchSeatMode::Online
			? TEXT("PLAYER 1")
			: (bLeftLocked ? TEXT("YOUR FIGHTER") : TEXT("CHOOSE YOUR FIGHTER"))))
		.Font(FBMenuArt::PosterFont(18, 50))
		.ColorAndOpacity(Quiet)
	];
	AddIdentity(LeftBody, PlayerCard);
	if (!bBuilderOpen)
	{
		LeftBody->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("KEYS TO WINNING")))
			.Font(FBMenuArt::PosterFont(11, 50))
			.ColorAndOpacity(Accent)
		];
		LeftBody->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(KeysToApproach(OpponentCard)))
			.Font(MenuFont(13))
			.ColorAndOpacity(Paper)
			.AutoWrapText(true)
		];
	}
	LeftBody->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 2.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("CHANGE FIGHTER")))
		.Font(FBMenuArt::PosterFont(11, 50))
		.ColorAndOpacity(Accent)
	];
	LeftBody->AddSlot().FillHeight(1.f)
	[
		SNew(SScrollBox) + SScrollBox::Slot()[ PlayerList ]
	];

	TSharedRef<SVerticalBox> RightBody = SNew(SVerticalBox);
	if (bBuilderOpen)
	{
		RightBody->AddSlot().AutoHeight()
		[
			SAssignNew(RightTitle, STextBlock)
			.Text(FText::FromString(TEXT("CREATE A FIGHTER")))
			.Font(FBMenuArt::PosterFont(18, 50))
			.ColorAndOpacity(Quiet)
		];
		RightBody->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Budget))
			.Font(MenuFont(13))
			.ColorAndOpacity(Quiet)
			.AutoWrapText(true)
		];
		RightBody->AddSlot().FillHeight(1.f)
		[
			SNew(SScrollBox) + SScrollBox::Slot()[ Builder ]
		];
		if (SaveRow.IsValid())
		{
			RightBody->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SaveRow.ToSharedRef()
			];
		}
	}
	else
	{
		RightBody->AddSlot().AutoHeight()
		[
			SAssignNew(RightTitle, STextBlock)
			.Text(FText::FromString(
				SeatMode == EFBMatchSeatMode::Online ? TEXT("PLAYER 2") :
				SeatMode == EFBMatchSeatMode::LocalVersus ? TEXT("PLAYER 2") :
				TEXT("CHOOSE AI OPPONENT")))
			.Font(FBMenuArt::PosterFont(18, 50))
			.ColorAndOpacity(Quiet)
		];
		AddIdentity(RightBody, OpponentCard);
		RightBody->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("KEYS TO WINNING")))
			.Font(FBMenuArt::PosterFont(11, 50))
			.ColorAndOpacity(Accent)
		];
		RightBody->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(KeysTheyUse(OpponentCard, PlayerCard)))
			.Font(MenuFont(13))
			.ColorAndOpacity(Paper)
			.AutoWrapText(true)
		];
		RightBody->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(SeatMode == EFBMatchSeatMode::LocalVersus && !bLocalSecondJoined
				? FString(TEXT("PRESS START ON THE SECOND CONTROLLER"))
				: FString::Printf(TEXT("CHANGE OPPONENT   ·   %s"), *ClassBlurb(WantedClass).ToUpper())))
			.Font(FBMenuArt::PosterFont(11, 16))
			.ColorAndOpacity(Accent)
			.AutoWrapText(true)
		];
		RightBody->AddSlot().FillHeight(1.f)
		[
			SNew(SScrollBox) + SScrollBox::Slot()[ OpponentList ]
		];
	}

	const FLinearColor PanelFill(0.1098f, 0.1294f, 0.1569f, 0.94f);
	const FLinearColor Line(0.2275f, 0.2039f, 0.1725f, 1.f);
	TSharedPtr<SBorder> LeftFrame;
	TSharedPtr<SBorder> RightFrame;
	TSharedPtr<SBorder> ActionFrame;
	auto WrapPanel = [&](TSharedPtr<SBorder>& Frame, TSharedRef<SWidget> Body) -> TSharedRef<SWidget>
	{
		return SAssignNew(Frame, SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(Line)
			.Padding(3.f)
			[
				SNew(SBorder)
				.BorderImage(White)
				.BorderBackgroundColor(PanelFill)
				.Padding(14.f)
				[
					Body
				]
			];
	};

	TSharedRef<SVerticalBox> ActionBody = SNew(SVerticalBox);
	if (!SelectStatus.IsEmpty())
	{
		ActionBody->AddSlot().AutoHeight().Padding(8.f, 2.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(SelectStatus))
			.Font(MenuFont(13))
			.ColorAndOpacity(Accent)
			.AutoWrapText(true)
		];
	}
	ActionBody->AddSlot().AutoHeight().Padding(8.f, 2.f, 8.f, 4.f)
	[
		DifficultyRow
	];
	ActionBody->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)[ FightRow ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)[ BackRow ]
		+ SHorizontalBox::Slot().FillWidth(1.3f).Padding(4.f, 0.f)[ CreateRow ]
	];

	TSharedRef<SWidget> LeftSlot = bBuilderOpen
		? StaticCastSharedRef<SWidget>(SNew(SFBCreatorViewport).Controller(this))
		: WrapPanel(LeftFrame, LeftBody);
	TSharedRef<SWidget> Panels = SNew(SConstraintCanvas)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SConstraintCanvas::Slot()
		.Anchors(bBuilderOpen ? FAnchors(0.02f, 0.02f, 0.46f, 0.80f) : FAnchors(0.025f, 0.035f, 0.492f, 0.80f))
		.Offset(FMargin(0.f))
		[
			LeftSlot
		]
		+ SConstraintCanvas::Slot()
		.Anchors(bBuilderOpen ? FAnchors(0.48f, 0.035f, 0.975f, 0.80f) : FAnchors(0.508f, 0.035f, 0.975f, 0.80f))
		.Offset(FMargin(0.f))
		[
			WrapPanel(RightFrame, RightBody)
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.025f, 0.815f, 0.975f, 0.975f))
		.Offset(FMargin(0.f))
		[
			SAssignNew(ActionFrame, SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(Line)
			.Padding(3.f)
			[
				SNew(SBorder)
				.BorderImage(White)
				.BorderBackgroundColor(PanelFill)
				.Padding(FMargin(8.f, 6.f))
				[
					ActionBody
				]
			]
		];

	TSharedRef<SFBFighterSelectStage> Stage = SNew(SFBFighterSelectStage)
		.Reveal(&SelectReveal)
		[
			Panels
		];

	const float BackdropLeft = bBuilderOpen ? 0.46f : 0.f;
	FighterSelectWidget = WrapForPad(SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(BackdropLeft, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		[
			SNew(SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor(0.012f, 0.014f, 0.018f, 1.f))
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(BackdropLeft, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage).Image(FBMenuArt::GetArenaBrush())
			]
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(BackdropLeft, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		[
			SNew(SButton)
			.ButtonStyle(&ClearButton())
			.OnClicked_Lambda([]() { return FReply::Handled(); })
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		[
			Stage
		]);

	SelectSides = FocusSides;
	auto Keep = [&](int32& Stored, int32 Side)
	{
		if (!SelectSides.IsValidIndex(Stored) || SelectSides[Stored] != Side)
		{
			const int32 First = FFBSelectNav::FirstOnSide(SelectSides, Side);
			Stored = First == INDEX_NONE ? 0 : First;
		}
	};
	Keep(LeftFocus, 0);
	Keep(RightFocus, 1);
	Keep(ActionFocus, 2);
	const int32 PaintSide = UsesSplitSeats()
		? (MatchSeat == 1 ? 1 : 0)
		: ActiveSide;
	SelectFocus = FocusOnSide(PaintSide);
	if (SelectRows.Num() > 0)
	{
		SelectFocus = FMath::Clamp(SelectFocus, 0, SelectRows.Num() - 1);
	}
	PaintSelectChrome();
	Stage->Watch(&SelectFocus, FocusSides, LeftFrame, RightFrame, ActionFrame, LeftTitle, RightTitle);
	LocalViewport()->AddViewportWidgetContent(FighterSelectWidget.ToSharedRef(), 100);
	FocusMenuWidget(FighterSelectWidget);
	if (bBuilderOpen)
	{
		if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
		{
			Boxer->PreviewCreatorLook(Draft.Look);
		}
	}
}
