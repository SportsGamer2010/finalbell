// Final Bell
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Input/FBRuntimeInputLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "InputActionValue.h"

AFBBoxerCharacter::AFBBoxerCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 320.f;
	bUseControllerRotationYaw = true;

	CombatComponent = CreateDefaultSubobject<UFBCombatComponent>(TEXT("CombatComponent"));
}

void AFBBoxerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFBBoxerCharacter, Stance);
	DOREPLIFETIME(AFBBoxerCharacter, AccumulatedDamage);
	DOREPLIFETIME(AFBBoxerCharacter, AppliedAttackInstanceIds);
}

void AFBBoxerCharacter::EnsureRuntimeInput()
{
	if (DefaultMappingContext && MoveAction && JabAction && CrossAction && GuardAction)
	{
		return;
	}

	const FFBRuntimeInputBundle Bundle = UFBRuntimeInputLibrary::EnsureBoxerInput(this);
	DefaultMappingContext = Bundle.MappingContext;
	MoveAction = Bundle.Move;
	JabAction = Bundle.Jab;
	CrossAction = Bundle.Cross;
	GuardAction = Bundle.Guard;
}

void AFBBoxerCharacter::ApplyMappingContext()
{
	EnsureRuntimeInput();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AFBBoxerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyMappingContext();
}

void AFBBoxerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	ApplyMappingContext();
}

void AFBBoxerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void AFBBoxerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureRuntimeInput();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFBBoxerCharacter::Move);
		}
		if (JabAction)
		{
			EIC->BindAction(JabAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnJab);
		}
		if (CrossAction)
		{
			EIC->BindAction(CrossAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnCross);
		}
		if (GuardAction)
		{
			EIC->BindAction(GuardAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnGuardStarted);
			EIC->BindAction(GuardAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnGuardCompleted);
			EIC->BindAction(GuardAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnGuardCompleted);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FinalBell: InputComponent is not EnhancedInputComponent. Check DefaultInput.ini."));
	}
}

void AFBBoxerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		AddMovementInput(Forward, Axis.Y * MoveSpeedScale);
		AddMovementInput(Right, Axis.X * MoveSpeedScale);
	}
}

void AFBBoxerCharacter::OnJab()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Jab, bGuard, bGuard ? EFBGuardState::High : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnCross()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Cross, bGuard, bGuard ? EFBGuardState::High : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnGuardStarted()
{
	SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::High);
}

void AFBBoxerCharacter::OnGuardCompleted()
{
	SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
}

void AFBBoxerCharacter::SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard)
{
	FFBCombatIntent Intent;
	Intent.Sequence = ++LocalIntentSequence;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = Attack;
	Intent.bGuardHeld = bGuardHeld;
	Intent.Guard = Guard;
	Intent.SchemaVersion = TEXT("1.0.0");

	if (HasAuthority())
	{
		if (CombatComponent)
		{
			CombatComponent->ServerTryProcessIntent(Intent, FindOpponent());
		}
	}
	else
	{
		ServerSubmitCombatIntent(Intent);
	}
}

bool AFBBoxerCharacter::ServerSubmitCombatIntent_Validate(const FFBCombatIntent& Intent)
{
	return Intent.Sequence > 0 && Intent.SchemaVersion.Len() > 0;
}

void AFBBoxerCharacter::ServerSubmitCombatIntent_Implementation(const FFBCombatIntent& Intent)
{
	if (CombatComponent)
	{
		CombatComponent->ServerTryProcessIntent(Intent, FindOpponent());
	}
}

AFBBoxerCharacter* AFBBoxerCharacter::FindOpponent() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
	{
		AFBBoxerCharacter* Other = *It;
		if (Other && Other != this)
		{
			return Other;
		}
	}
	return nullptr;
}

void AFBBoxerCharacter::ApplyAuthoritativeDamage(float Amount, int32 AttackInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}
	if (AppliedAttackInstanceIds.Contains(AttackInstanceId))
	{
		return;
	}
	AppliedAttackInstanceIds.Add(AttackInstanceId);
	AccumulatedDamage += FMath::Max(0.f, Amount);
}
