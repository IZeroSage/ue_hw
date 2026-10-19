#include "MechanicsCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "HW1.h"

AMechanicsCharacter::AMechanicsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
	Movement->JumpZVelocity = 500.f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MinAnalogWalkSpeed = 20.f;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMechanicsCharacter::BeginPlay()
{
	Super::BeginPlay();
	Stamina = MaxStamina;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AMechanicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(Input, TEXT("MechanicsCharacter requires Enhanced Input")))
	{
		return;
	}

	if (JumpAction)
	{
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AMechanicsCharacter::DoJumpStart);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMechanicsCharacter::DoJumpEnd);
		Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AMechanicsCharacter::DoJumpEnd);
	}
	if (MoveAction) Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMechanicsCharacter::Move);
	if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMechanicsCharacter::Look);
	if (MouseLookAction) Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMechanicsCharacter::Look);
	if (ensure(SprintAction))
	{
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AMechanicsCharacter::StartSprint);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMechanicsCharacter::StopSprint);
		Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMechanicsCharacter::StopSprint);
	}
	if (ensure(InteractAction))
	{
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AMechanicsCharacter::Interact);
	}
}

void AMechanicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSprintState();
	if (bIsSprinting)
	{
		TimeSinceSprint = 0.f;
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainPerSecond * DeltaSeconds);
		if (Stamina <= 0.f)
		{
			bSprintNeedsRelease = true;
			UpdateSprintState();
		}
	}
	else
	{
		// Account only for the part of this frame after the recovery delay.
		const float PreviousTime = TimeSinceSprint;
		TimeSinceSprint += DeltaSeconds;
		const float RecoverySeconds = FMath::Max(0.f, TimeSinceSprint - RecoveryDelay)
			- FMath::Max(0.f, PreviousTime - RecoveryDelay);
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRecoveryPerSecond * RecoverySeconds);
	}
}

void AMechanicsCharacter::UpdateSprintState()
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bMoving = !Movement->GetCurrentAcceleration().IsNearlyZero();
	bIsSprinting = bSprintHeld && !bSprintNeedsRelease && Stamina > 0.f
		&& Movement->IsMovingOnGround() && bMoving;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

void AMechanicsCharacter::StartSprint()
{
	bSprintHeld = true;
	if (Stamina < FMath::Min(MinimumSprintStamina, MaxStamina)) bSprintNeedsRelease = true;
	UpdateSprintState();
}

void AMechanicsCharacter::StopSprint()
{
	bSprintHeld = false;
	bSprintNeedsRelease = false;
	UpdateSprintState();
}

void AMechanicsCharacter::Interact()
{
	OnInteractionTriggered.Broadcast();
	UE_LOG(LogHW1, Display, TEXT("Interaction delegate broadcast (Blueprint bound: %s)"),
		OnInteractionTriggered.IsBound() ? TEXT("yes") : TEXT("no"));
}

void AMechanicsCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoMove(Axis.X, Axis.Y);
}

void AMechanicsCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	DoLook(Axis.X, Axis.Y);
}

void AMechanicsCharacter::DoMove(float Right, float Forward)
{
	if (Controller)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
	}
}

void AMechanicsCharacter::DoLook(float Yaw, float Pitch)
{
	if (Controller)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMechanicsCharacter::DoJumpStart() { Jump(); }
void AMechanicsCharacter::DoJumpEnd() { StopJumping(); }
