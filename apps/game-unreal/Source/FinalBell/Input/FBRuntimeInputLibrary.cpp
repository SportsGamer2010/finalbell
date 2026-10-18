#include "Input/FBRuntimeInputLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float StickDeadZone = 0.38f;

	UInputAction* MakeAction(UObject* Outer, const FName Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		if (Type == EInputActionValueType::Boolean)
		{
			// Without an explicit trigger, some keys never fire Started.
			UInputTriggerPressed* Pressed = NewObject<UInputTriggerPressed>(Action);
			Action->Triggers.Add(Pressed);
		}
		return Action;
	}

	void AddRadialDeadZone(FEnhancedActionKeyMapping& Mapping, UObject* Outer, float Threshold = StickDeadZone)
	{
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(Outer);
		DeadZone->LowerThreshold = Threshold;
		DeadZone->UpperThreshold = 1.f;
		DeadZone->Type = EDeadZoneType::Radial;
		Mapping.Modifiers.Add(DeadZone);
	}

	void MapAxisKey(UInputMappingContext* IMC, UInputAction* Action, const FKey& Key, bool bSwizzleY, bool bNegate, bool bApplyDeadZone)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Action, Key);
		if (bApplyDeadZone)
		{
			AddRadialDeadZone(Mapping, IMC);
		}
		if (bSwizzleY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(IMC));
		}
	}
}

FFBRuntimeInputBundle UFBRuntimeInputLibrary::EnsureBoxerInput(UObject* Outer)
{
	FFBRuntimeInputBundle Bundle;
	if (!Outer)
	{
		return Bundle;
	}

	Bundle.Move = MakeAction(Outer, TEXT("IA_FB_Move"), EInputActionValueType::Axis2D);
	Bundle.Look = MakeAction(Outer, TEXT("IA_FB_Look"), EInputActionValueType::Axis2D);
	Bundle.PunchStick = MakeAction(Outer, TEXT("IA_FB_PunchStick"), EInputActionValueType::Axis2D);
	Bundle.Turn = MakeAction(Outer, TEXT("IA_FB_Turn"), EInputActionValueType::Axis2D);
	Bundle.Jab = MakeAction(Outer, TEXT("IA_FB_Jab"), EInputActionValueType::Boolean);
	Bundle.Cross = MakeAction(Outer, TEXT("IA_FB_Cross"), EInputActionValueType::Boolean);
	Bundle.LeadHook = MakeAction(Outer, TEXT("IA_FB_LeadHook"), EInputActionValueType::Boolean);
	Bundle.RearHook = MakeAction(Outer, TEXT("IA_FB_RearHook"), EInputActionValueType::Boolean);
	Bundle.Block = MakeAction(Outer, TEXT("IA_FB_Block"), EInputActionValueType::Boolean);
	Bundle.Weave = MakeAction(Outer, TEXT("IA_FB_Weave"), EInputActionValueType::Boolean);
	Bundle.BodyMod = MakeAction(Outer, TEXT("IA_FB_BodyMod"), EInputActionValueType::Boolean);
	Bundle.PowerMod = MakeAction(Outer, TEXT("IA_FB_PowerMod"), EInputActionValueType::Boolean);
	Bundle.MappingContext = NewObject<UInputMappingContext>(Outer, TEXT("IMC_FB_Boxer_UndisputedCompetitor"));

	// Move — left stick / WASD
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::W, true, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::S, true, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::D, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::A, false, true, false);
	{
		FEnhancedActionKeyMapping& Left2D = Bundle.MappingContext->MapKey(Bundle.Move, EKeys::Gamepad_Left2D);
		AddRadialDeadZone(Left2D, Bundle.MappingContext);
	}
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftY, true, false, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftX, false, false, true);

	// Camera orbit — mouse + Q/E only (RS reserved for Undisputed punch flick)
	{
		FEnhancedActionKeyMapping& Mouse = Bundle.MappingContext->MapKey(Bundle.Look, EKeys::Mouse2D);
		if (UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Bundle.MappingContext))
		{
			Scalar->Scalar = FVector(0.045f, 0.045f, 0.045f);
			Mouse.Modifiers.Add(Scalar);
		}
	}
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::E, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::Q, false, true, false);

	// Right stick angles the shoulders. A hard flick still throws the punch once.
	{
		FEnhancedActionKeyMapping& Turn2D = Bundle.MappingContext->MapKey(Bundle.Turn, EKeys::Gamepad_Right2D);
		AddRadialDeadZone(Turn2D, Bundle.MappingContext, 0.12f);
	}
	{
		FEnhancedActionKeyMapping& Right2D = Bundle.MappingContext->MapKey(Bundle.PunchStick, EKeys::Gamepad_Right2D);
		AddRadialDeadZone(Right2D, Bundle.MappingContext, 0.78f);
	}

	// Face buttons — Competitor: Square/X jab, Triangle/Y cross, X/A lead hook, Circle/B rear hook
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Left);   // Square / X
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::J);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::LeftMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::One);

	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_FaceButton_Top);  // Triangle / Y
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::K);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::RightMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Two);

	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::Gamepad_FaceButton_Bottom); // X / A
	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::U);
	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::Three);

	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::Gamepad_FaceButton_Right); // Circle / B
	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::I);
	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::Four);

	Bundle.LeadUppercut = MakeAction(Outer, TEXT("IA_FB_LeadUppercut"), EInputActionValueType::Boolean);
	Bundle.RearUppercut = MakeAction(Outer, TEXT("IA_FB_RearUppercut"), EInputActionValueType::Boolean);
	Bundle.MappingContext->MapKey(Bundle.LeadUppercut, EKeys::Y);
	Bundle.MappingContext->MapKey(Bundle.LeadUppercut, EKeys::Five);
	Bundle.MappingContext->MapKey(Bundle.RearUppercut, EKeys::H);
	Bundle.MappingContext->MapKey(Bundle.RearUppercut, EKeys::Six);

	// Block R2 / RT / Shift ; Weave L2 / LT / Ctrl
	Bundle.MappingContext->MapKey(Bundle.Block, EKeys::Gamepad_RightTrigger);
	Bundle.MappingContext->MapKey(Bundle.Block, EKeys::LeftShift);
	Bundle.MappingContext->MapKey(Bundle.Weave, EKeys::Gamepad_LeftTrigger);
	Bundle.MappingContext->MapKey(Bundle.Weave, EKeys::LeftControl);

	// Modifiers: L1/C body, R1/V power
	Bundle.MappingContext->MapKey(Bundle.BodyMod, EKeys::Gamepad_LeftShoulder);
	Bundle.MappingContext->MapKey(Bundle.BodyMod, EKeys::C);
	Bundle.MappingContext->MapKey(Bundle.PowerMod, EKeys::Gamepad_RightShoulder);
	Bundle.MappingContext->MapKey(Bundle.PowerMod, EKeys::V);

	return Bundle;
}
