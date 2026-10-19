#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "MechanicsCharacter.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/OutputDevice.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PlayInEditorDataTypes.h"
#include "Serialization/JsonSerializer.h"
#include "StaminaHUD.h"
#include "UnrealClient.h"

/** End-to-end checks against a fresh PIE instance and the saved assets.
 * Sends simulated key events through PlayerController/Enhanced Input rather than calling gameplay handlers.
 */
class FHomeworkVerification : public FOutputDevice
{
public:
	~FHomeworkVerification() { if (GLog) GLog->RemoveOutputDevice(this); }

	virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (Category == TEXT("LogBlueprintUserMessages") && FString(Message).Contains(TEXT("Interaction triggered!")))
			bSawBlueprintMessage = true;
	}

	bool Tick(float)
	{
		if (bFinished)
		{
			if (GEditor && GEditor->PlayWorld) return true;
			FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
			return false;
		}
		if (FPlatformTime::Seconds() - StartedAt > 150.)
		{
			Check(TEXT("Verification finished before timeout"), false);
			return Finish();
		}
		if (!GEditor || !GEditor->GetEditorWorldContext().World()) return true;
		if (!bRequestedPIE)
		{
			bRequestedPIE = true;
			GLog->AddOutputDevice(this);
			FRequestPlaySessionParams Params;
			Params.GlobalMapOverride = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
			Params.bAllowOnlineSubsystem = false;
			GEditor->RequestPlaySession(Params);
			return true;
		}
		UWorld* World = GEditor->PlayWorld;
		if (!World) return true;
		APlayerController* Controller = World->GetFirstPlayerController();
		AMechanicsCharacter* Character = Controller ? Cast<AMechanicsCharacter>(Controller->GetPawn()) : nullptr;
		if (!Controller || !Character)
		{
			if (World->GetTimeSeconds() > 5.f)
			{
				Check(TEXT("Default pawn uses MechanicsCharacter"), false);
				return Finish();
			}
			return true;
		}
		Player = Controller;
		Character->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		const float Now = World->GetTimeSeconds();
		const float Elapsed = Now - StageStartedAt;
		auto* Movement = Character->GetCharacterMovement();
		auto* Anim = Character->GetMesh()->GetAnimInstance();
		const int32 Machine = Anim ? Anim->GetStateMachineIndex(TEXT("Locomotion")) : INDEX_NONE;
		const FString State = Anim && Machine != INDEX_NONE ? Anim->GetCurrentStateName(Machine).ToString() : TEXT("<missing>");

		switch (Stage)
		{
		case 0:
			if (Now < 0.8f) break;
			Check(TEXT("Character starts with full stamina"), FMath::IsNearlyEqual(Character->Stamina, Character->MaxStamina));
			Check(TEXT("Follow camera preserved"), Character->FindComponentByClass<UCameraComponent>() != nullptr);
			Check(TEXT("Persistent stamina HUD active"), Cast<AStaminaHUD>(Controller->GetHUD()) != nullptr);
			Check(TEXT("Blueprint interaction delegate bound"), Character->OnInteractionTriggered.IsBound());
			Check(TEXT("Original locomotion state machine present"), Machine != INDEX_NONE);
			Origin = Character->GetActorLocation();
			Key(EKeys::W, true);
			Next(Now);
			break;
		case 1:
			if (Elapsed < 0.7f) break;
			Check(TEXT("W moves the character through Enhanced Input"), FVector::Dist2D(Origin, Character->GetActorLocation()) > 50.f);
			Check(TEXT("Normal movement keeps walking speed"), !Character->bIsSprinting && FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Character->WalkSpeed));
			Key(EKeys::LeftShift, true);
			Next(Now);
			break;
		case 2:
			if (Elapsed > 0.4f && !bCapturedSprint)
			{
				Screenshot(TEXT("Sprint.png"));
				bCapturedSprint = true;
			}
			if (Elapsed < 0.8f) break;
			Check(TEXT("Shift starts sprint"), Character->bIsSprinting && FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Character->SprintSpeed));
			Check(TEXT("Sprint consumes stamina"), Character->Stamina < Character->MaxStamina - 5.f);
			Check(TEXT("Animation enters real Sprint state"), State == TEXT("Sprint"));
			Key(EKeys::LeftShift, false);
			Next(Now);
			break;
		case 3:
			if (Elapsed < 0.5f) break;
			Check(TEXT("Releasing Shift restores walking speed"), !Character->bIsSprinting && FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Character->WalkSpeed));
			Check(TEXT("Animation exits Sprint"), State != TEXT("Sprint") && Machine != INDEX_NONE);
			Key(EKeys::E, true);
			Next(Now);
			break;
		case 4:
			if (Elapsed > 0.05f && !bCapturedInteraction)
			{
				Screenshot(TEXT("Interaction.png"));
				bCapturedInteraction = true;
			}
			if (Elapsed < 0.2f) break;
			Key(EKeys::E, false);
			Check(TEXT("E broadcasts delegate and executes Blueprint Print String"), bSawBlueprintMessage);
			Key(EKeys::LeftShift, true);
			Next(Now);
			break;
		case 5:
			if (Character->IsSprintLocked())
			{
				Check(TEXT("Stamina depletion stops sprint at zero"), Character->Stamina <= 0.1f && !Character->bIsSprinting);
				Next(Now);
			}
			else if (Elapsed > 7.f)
			{
				Check(TEXT("Stamina depletion stops sprint at zero"), false);
				Next(Now);
			}
			break;
		case 6:
			if (Elapsed < 2.5f) break;
			Check(TEXT("Holding Shift after depletion cannot restart sprint"), !Character->bIsSprinting && Character->IsSprintLocked());
			Check(TEXT("Stamina recovers while exhausted"), Character->Stamina > 10.f && Character->Stamina <= Character->MaxStamina);
			Check(TEXT("Exhaustion restores walk speed and animation"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Character->WalkSpeed) && State != TEXT("Sprint"));
			Key(EKeys::LeftShift, false);
			Key(EKeys::W, false);
			RecoveryStart = Character->Stamina;
			Next(Now);
			break;
		case 7:
			if (Elapsed < 5.f) break;
			Check(TEXT("Rest regenerates stamina without exceeding maximum"), Character->Stamina > RecoveryStart && Character->Stamina <= Character->MaxStamina);
			Check(TEXT("Idle does not sprint"), !Character->bIsSprinting);
			Key(EKeys::S, true);
			Key(EKeys::LeftShift, true);
			Next(Now);
			break;
		case 8:
			if (Elapsed < 0.8f) break;
			Check(TEXT("Fresh Shift press after recovery resumes sprint"), Character->bIsSprinting && State == TEXT("Sprint"));
			Key(EKeys::SpaceBar, true);
			Next(Now);
			break;
		case 9:
			if (Elapsed < 0.2f) break;
			Key(EKeys::SpaceBar, false);
			Check(TEXT("Space jump remains functional"), Movement->IsFalling() && Character->GetVelocity().Z > 0.f);
			Check(TEXT("Jump animation remains functional"), Anim && Anim->GetCurrentStateName(Anim->GetStateMachineIndex(TEXT("Main States"))) == TEXT("Jump"));
			Key(EKeys::LeftShift, false);
			Key(EKeys::S, false);
			Next(Now);
			break;
		case 10:
			if (Elapsed < 2.f) break;
			Check(TEXT("Character lands normally"), Movement->IsMovingOnGround());
			Check(TEXT("Landing returns to locomotion"), Anim && Anim->GetCurrentStateName(Anim->GetStateMachineIndex(TEXT("Main States"))) == TEXT("Locomotion"));
			return Finish();
		}
		return true;
	}

private:
	void Screenshot(const TCHAR* Filename)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Verification") / Filename, false, false);
	}
	void Key(const FKey& KeyName, bool bDown)
	{
		if (Player.IsValid()) Player->InputKey(FInputKeyEventArgs::CreateSimulated(
			KeyName, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
	}
	void Next(float Now) { ++Stage; StageStartedAt = Now; }
	void Check(const TCHAR* Name, bool bSuccess)
	{
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("check"), Name);
		Row->SetBoolField(TEXT("passed"), bSuccess);
		Checks.Add(MakeShared<FJsonValueObject>(Row));
		bPassed &= bSuccess;
		UE_LOG(LogTemp, Display, TEXT("HW1 VERIFY %s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Name);
	}
	bool Finish()
	{
		Key(EKeys::W, false); Key(EKeys::S, false); Key(EKeys::LeftShift, false); Key(EKeys::E, false); Key(EKeys::SpaceBar, false);
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("passed"), bPassed);
		Root->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
		Root->SetStringField(TEXT("method"), TEXT("Fresh PIE session; simulated key events through PlayerController and Enhanced Input"));
		Root->SetArrayField(TEXT("checks"), Checks);
		FString Json;
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Verification");
		IFileManager::Get().MakeDirectory(*Directory, true);
		FFileHelper::SaveStringToFile(Json, *(Directory / TEXT("gameplay.json")));
		GLog->RemoveOutputDevice(this);
		if (GEditor && GEditor->PlayWorld) GEditor->RequestEndPlayMap();
		bFinished = true;
		return true;
	}

	double StartedAt = FPlatformTime::Seconds();
	float StageStartedAt = 0.f;
	float RecoveryStart = 0.f;
	int32 Stage = 0;
	bool bRequestedPIE = false;
	bool bFinished = false;
	bool bPassed = true;
	bool bSawBlueprintMessage = false;
	bool bCapturedSprint = false;
	bool bCapturedInteraction = false;
	FVector Origin;
	TWeakObjectPtr<APlayerController> Player;
	TArray<TSharedPtr<FJsonValue>> Checks;
};

void StartHomeworkVerification()
{
	TSharedRef<FHomeworkVerification> Verification = MakeShared<FHomeworkVerification>();
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[Verification](float DeltaSeconds) { return Verification->Tick(DeltaSeconds); }));
}
