#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MechanicsEditorLibrary.generated.h"

/** Editor-only asset authoring and verification helpers; never part of the game. */
UCLASS()
class HW1EDITOR_API UMechanicsEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Homework|Editor")
	static FString InspectProject();

	UFUNCTION(BlueprintCallable, Category="Homework|Editor")
	static bool BuildHomeworkAssets();
};
