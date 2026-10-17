#include "Input/FBRuntimeInputLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

namespace
{
	UInputAction* MakeAction(UObject* Outer, const FName Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		return Action;
	}

	void MapAxisKey(UInputMappingContext* IMC, UInputAction* Action, const FKey& Key, bool bSwizzleY, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Action, Key);
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
	Bundle.Jab = MakeAction(Outer, TEXT("IA_FB_Jab"), EInputActionValueType::Boolean);
	Bundle.Cross = MakeAction(Outer, TEXT("IA_FB_Cross"), EInputActionValueType::Boolean);
	Bundle.Guard = MakeAction(Outer, TEXT("IA_FB_Guard"), EInputActionValueType::Boolean);
	Bundle.MappingContext = NewObject<UInputMappingContext>(Outer, TEXT("IMC_FB_Boxer"));

	// Keyboard WASD
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::W, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::S, true, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::D, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::A, false, true);

	// Gamepad stick + axes + D-pad fallback
	Bundle.MappingContext->MapKey(Bundle.Move, EKeys::Gamepad_Left2D);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftY, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftX, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Up, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Down, true, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Right, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Left, false, true);

	// Jab: Square/X, Cross/A, R2
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::LeftMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::J);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Left);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Bottom);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_RightTrigger);

	// Cross: Triangle/Y, R1
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::RightMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::K);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_FaceButton_Top);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_RightShoulder);

	// Guard: L1 / L2
	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::LeftShift);
	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::Gamepad_LeftShoulder);
	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::Gamepad_LeftTrigger);

	return Bundle;
}
