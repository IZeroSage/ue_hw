#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MechanicsCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractionTriggered);

/** Homework character. Gameplay and Enhanced Input bindings live in C++. */
UCLASS()
class HW1_API AMechanicsCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMechanicsCharacter();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FInteractionTriggered OnInteractionTriggered;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Sprint")
	bool bIsSprinting = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sprint", meta=(ClampMin="1"))
	float WalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sprint", meta=(ClampMin="1"))
	float SprintSpeed = 850.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="1"))
	float MaxStamina = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stamina")
	float Stamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float StaminaDrainPerSecond = 22.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float StaminaRecoveryPerSecond = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float RecoveryDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float MinimumSprintStamina = 15.f;

	UFUNCTION(BlueprintPure, Category="Stamina")
	bool IsSprintLocked() const { return bSprintNeedsRelease; }

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoJumpEnd();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void Interact();
	void UpdateSprintState();

	bool bSprintHeld = false;
	// Exhaustion cannot retrigger sprint while Shift remains held, even after recovery.
	bool bSprintNeedsRelease = false;
	float TimeSinceSprint = 0.f;
};
