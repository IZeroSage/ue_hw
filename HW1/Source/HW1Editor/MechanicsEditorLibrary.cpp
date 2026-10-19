#include "MechanicsEditorLibrary.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Character.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/JsonSerializer.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationTransitionGraph.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintEditorLibrary.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/GameModeBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MechanicsCharacter.h"
#include "Misc/PackageName.h"
#include "StaminaHUD.h"
#include "UObject/SavePackage.h"

namespace HomeworkAssets
{
	template<class T, class F>
	T* Node(UEdGraph* Graph, int32 X, int32 Y, F Configure)
	{
		FGraphNodeCreator<T> Creator(*Graph);
		T* Result = Creator.CreateNode();
		Result->NodePosX = X;
		Result->NodePosY = Y;
		Configure(Result);
		Creator.Finalize();
		return Result;
	}

	void Link(UEdGraphPin* From, UEdGraphPin* To)
	{
		check(From && To);
		checkf(From->GetOwningNode()->GetSchema()->TryCreateConnection(From, To),
			TEXT("Cannot connect %s.%s to %s.%s"), *From->GetOwningNode()->GetName(),
			*From->PinName.ToString(), *To->GetOwningNode()->GetName(), *To->PinName.ToString());
	}

	bool Save(UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args);
	}

	UInputAction* Action(const TCHAR* PackagePath)
	{
		UInputAction* Result = LoadObject<UInputAction>(nullptr, PackagePath);
		if (!Result)
		{
			UPackage* Package = CreatePackage(PackagePath);
			Result = NewObject<UInputAction>(Package, *FPackageName::GetShortName(PackagePath), RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Result);
		}
		Result->ValueType = EInputActionValueType::Boolean;
		return Result;
	}

	UK2Node_CallFunction* Call(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
	{
		return Node<UK2Node_CallFunction>(Graph, X, Y, [Function](auto* N) { N->SetFromFunction(Function); });
	}

	UK2Node_VariableGet* Get(UEdGraph* Graph, FName Name, int32 X, int32 Y, UClass* Owner = nullptr)
	{
		return Node<UK2Node_VariableGet>(Graph, X, Y, [=](auto* N)
		{
			if (Owner) N->VariableReference.SetExternalMember(Name, Owner);
			else N->VariableReference.SetSelfMember(Name);
		});
	}

	void AddTransition(UAnimStateNode* From, UAnimStateNode* To, bool bNegate)
	{
		UAnimStateTransitionNode* Transition = Node<UAnimStateTransitionNode>(From->GetGraph(),
			(From->NodePosX + To->NodePosX) / 2, bNegate ? 180 : -100, [](auto*) {});
		Transition->CrossfadeDuration = 0.18f;
		Transition->CreateConnections(From, To);
		UEdGraph* Rule = Transition->BoundGraph;
		UK2Node_VariableGet* Sprint = Get(Rule, TEXT("bIsSprinting"), -400, 0);
		UEdGraphPin* Value = Sprint->FindPinChecked(TEXT("bIsSprinting"));
		if (bNegate)
		{
			auto* Not = Call(Rule, UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Not_PreBool")), -200, 0);
			Link(Value, Not->FindPinChecked(TEXT("A")));
			Value = Not->GetReturnValuePin();
		}
		auto* Result = CastChecked<UAnimationTransitionGraph>(Rule)->GetResultNode();
		Link(Value, Result->FindPinChecked(TEXT("bCanEnterTransition")));
	}
}

bool UMechanicsEditorLibrary::BuildHomeworkAssets()
{
	using namespace HomeworkAssets;
	UBlueprint* CharacterBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	UBlueprint* GameModeBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode"));
	UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default"));
	if (!CharacterBP || !GameModeBP || !AnimBP || !Context) return false;

	UInputAction* SprintAction = Action(TEXT("/Game/Input/Actions/IA_Sprint"));
	UInputAction* InteractAction = Action(TEXT("/Game/Input/Actions/IA_Interact"));
	Context->UnmapAllKeysFromAction(SprintAction);
	Context->UnmapAllKeysFromAction(InteractAction);
	Context->MapKey(SprintAction, EKeys::LeftShift);
	Context->MapKey(InteractAction, EKeys::E);

	ACharacter* OldCDO = CastChecked<ACharacter>(CharacterBP->GeneratedClass->GetDefaultObject());
	USkeletalMesh* Mesh = OldCDO->GetMesh()->GetSkeletalMeshAsset();
	const FTransform MeshTransform = OldCDO->GetMesh()->GetRelativeTransform();
	if (CharacterBP->ParentClass != AMechanicsCharacter::StaticClass())
	{
		UBlueprintEditorLibrary::ReparentBlueprint(CharacterBP, AMechanicsCharacter::StaticClass());
		TArray<UEdGraph*> Graphs;
		CharacterBP->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
			for (UEdGraphNode* GraphNode : Graph->Nodes)
				if (UK2Node_CallFunction* N = Cast<UK2Node_CallFunction>(GraphNode))
					if (N->FunctionReference.GetMemberName().ToString().StartsWith(TEXT("Do")))
					{
						N->FunctionReference.SetSelfMember(N->FunctionReference.GetMemberName());
						N->ReconstructNode();
					}
	}

	UEdGraph* CharacterGraph = FBlueprintEditorUtils::FindEventGraph(CharacterBP);
	bool bHasBinding = false;
	for (UEdGraphNode* N : CharacterGraph->Nodes) bHasBinding |= N->NodeComment == TEXT("HW1: Bind interaction delegate");
	if (!bHasBinding)
	{
		auto* Begin = Node<UK2Node_Event>(CharacterGraph, 0, 500, [](auto* N)
		{
			N->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"), AActor::StaticClass());
			N->bOverrideFunction = true;
		});
		auto* Bind = Node<UK2Node_AddDelegate>(CharacterGraph, 320, 500, [](auto* N)
		{
			N->DelegateReference.SetSelfMember(TEXT("OnInteractionTriggered"));
			N->NodeComment = TEXT("HW1: Bind interaction delegate");
			N->bCommentBubbleVisible = true;
		});
		auto* Handler = UK2Node_CustomEvent::CreateFromFunction(FVector2D(0, 800), CharacterGraph,
			TEXT("HandleInteraction"), Bind->GetDelegateSignature(), false);
		auto* Print = Call(CharacterGraph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString")), 640, 800);
		Print->FindPinChecked(TEXT("InString"))->DefaultValue = TEXT("Interaction triggered!");
		Print->FindPinChecked(TEXT("Duration"))->DefaultValue = TEXT("2.0");
		Print->FindPinChecked(TEXT("bPrintToScreen"))->DefaultValue = TEXT("true");
		Print->FindPinChecked(TEXT("bPrintToLog"))->DefaultValue = TEXT("true");
		Link(Begin->FindPinChecked(UEdGraphSchema_K2::PN_Then), Bind->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
		Link(Handler->FindPinChecked(TEXT("OutputDelegate")), Bind->GetDelegatePin());
		Link(Handler->FindPinChecked(UEdGraphSchema_K2::PN_Then), Print->GetExecPin());
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(CharacterBP);
	FKismetEditorUtilities::CompileBlueprint(CharacterBP);
	if (CharacterBP->Status == BS_Error) return false;
	AMechanicsCharacter* CDO = CastChecked<AMechanicsCharacter>(CharacterBP->GeneratedClass->GetDefaultObject());
	CDO->GetMesh()->SetSkeletalMesh(Mesh);
	CDO->GetMesh()->SetRelativeTransform(MeshTransform);
	CDO->GetMesh()->SetAnimInstanceClass(AnimBP->GeneratedClass);
	for (FName Name : { FName("JumpAction"), FName("MoveAction"), FName("LookAction"), FName("MouseLookAction") })
	{
		FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(AMechanicsCharacter::StaticClass(), Name);
		const FString Path = TEXT("/Game/Input/Actions/IA_") + Name.ToString().LeftChop(6);
		Property->SetObjectPropertyValue_InContainer(CDO, LoadObject<UInputAction>(nullptr, *Path));
	}
	CDO->SprintAction = SprintAction;
	CDO->InteractAction = InteractAction;
	FBlueprintEditorUtils::MarkBlueprintAsModified(CharacterBP);

	if (FBlueprintEditorUtils::FindNewVariableIndex(AnimBP, TEXT("bIsSprinting")) == INDEX_NONE)
	{
		FEdGraphPinType Type;
		Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		FBlueprintEditorUtils::AddMemberVariable(AnimBP, TEXT("bIsSprinting"), Type);
		FBlueprintEditorUtils::SetBlueprintVariableCategory(AnimBP, TEXT("bIsSprinting"), nullptr, FText::FromString(TEXT("Sprint")));
	}
	UEdGraph* AnimEventGraph = FBlueprintEditorUtils::FindEventGraph(AnimBP);
	bool bHasSprintRead = false;
	for (UEdGraphNode* N : AnimEventGraph->Nodes) bHasSprintRead |= N->NodeComment == TEXT("HW1: Read C++ sprint state");
	if (!bHasSprintRead)
	{
		// Run after the template's falling-state update, preserving every existing animation branch.
		UK2Node_VariableSet* FallingSet = nullptr;
		for (UEdGraphNode* N : AnimEventGraph->Nodes)
			if (auto* Set = Cast<UK2Node_VariableSet>(N))
				if (Set->VariableReference.GetMemberName() == TEXT("IsFalling")) FallingSet = Set;
		if (!FallingSet) return false;
		auto* Character = Get(AnimEventGraph, TEXT("Character"), 2000, 1600);
		auto* CastNode = Node<UK2Node_DynamicCast>(AnimEventGraph, 2250, 1450, [](auto* N)
		{
			N->TargetType = AMechanicsCharacter::StaticClass();
			N->NodeComment = TEXT("HW1: Read C++ sprint state");
			N->bCommentBubbleVisible = true;
		});
		auto* Sprint = Get(AnimEventGraph, TEXT("bIsSprinting"), 2500, 1600, AMechanicsCharacter::StaticClass());
		auto* Set = Node<UK2Node_VariableSet>(AnimEventGraph, 2800, 1450,
			[](auto* N) { N->VariableReference.SetSelfMember(TEXT("bIsSprinting")); });
		Link(FallingSet->FindPinChecked(UEdGraphSchema_K2::PN_Then), CastNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
		Link(Character->FindPinChecked(TEXT("Character")), CastNode->GetCastSourcePin());
		Link(CastNode->GetCastResultPin(), Sprint->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		Link(CastNode->GetValidCastPin(), Set->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
		Link(Sprint->FindPinChecked(TEXT("bIsSprinting")), Set->FindPinChecked(TEXT("bIsSprinting")));
	}

	TArray<UEdGraph*> AnimGraphs;
	AnimBP->GetAllGraphs(AnimGraphs);
	UAnimationStateMachineGraph* Locomotion = nullptr;
	for (UEdGraph* Graph : AnimGraphs)
		if (Graph->GetFName() == TEXT("Locomotion"))
			if (auto* StateMachine = Cast<UAnimationStateMachineGraph>(Graph)) Locomotion = StateMachine;
	if (!Locomotion) return false;
	UAnimStateNode* Walk = nullptr;
	UAnimStateNode* Sprint = nullptr;
	for (UEdGraphNode* N : Locomotion->Nodes)
		if (auto* State = Cast<UAnimStateNode>(N))
		{
			if (State->GetStateName() == TEXT("Walk / Run")) Walk = State;
			if (State->GetStateName() == TEXT("Sprint")) Sprint = State;
		}
	if (!Walk) return false;
	if (!Sprint)
	{
		Sprint = Node<UAnimStateNode>(Locomotion, Walk->NodePosX + 420, Walk->NodePosY, [](auto*) {});
		FBlueprintEditorUtils::RenameGraph(Sprint->BoundGraph, TEXT("Sprint"));
		auto* Player = Node<UAnimGraphNode_SequencePlayer>(Sprint->BoundGraph, -300, 0, [](auto* N)
		{
			N->Node.SetSequence(LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd")));
			N->Node.SetLoopAnimation(true);
			N->Node.SetPlayRate(1.45f);
		});
		Link(Player->FindPinChecked(TEXT("Pose")), Sprint->GetPoseSinkPinInsideState());
		AddTransition(Walk, Sprint, false);
		AddTransition(Sprint, Walk, true);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);
	if (AnimBP->Status == BS_Error) return false;

	FKismetEditorUtilities::CompileBlueprint(GameModeBP);
	AGameModeBase* GameMode = CastChecked<AGameModeBase>(GameModeBP->GeneratedClass->GetDefaultObject());
	GameMode->DefaultPawnClass = CharacterBP->GeneratedClass;
	GameMode->HUDClass = AStaminaHUD::StaticClass();
	FBlueprintEditorUtils::MarkBlueprintAsModified(GameModeBP);
	return Save(SprintAction) && Save(InteractAction) && Save(Context)
		&& Save(CharacterBP) && Save(AnimBP) && Save(GameModeBP);
}

FString UMechanicsEditorLibrary::InspectProject()
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	UBlueprint* CharacterBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter"));
	if (!CharacterBP)
	{
		return TEXT("{\"error\":\"Character Blueprint missing\"}");
	}
	Root->SetStringField(TEXT("character_parent"), GetPathNameSafe(CharacterBP->ParentClass));
	ACharacter* CDO = Cast<ACharacter>(CharacterBP->GeneratedClass->GetDefaultObject());
	UClass* AnimClass = CDO->GetMesh()->GetAnimClass();
	Root->SetStringField(TEXT("animation_class"), GetPathNameSafe(AnimClass));
	Root->SetStringField(TEXT("mesh"), GetPathNameSafe(CDO->GetMesh()->GetSkeletalMeshAsset()));

	TArray<UBlueprint*> Blueprints = { CharacterBP };
	if (AnimClass)
	{
		if (UBlueprint* AnimBP = Cast<UBlueprint>(AnimClass->ClassGeneratedBy)) Blueprints.Add(AnimBP);
	}
	TArray<TSharedPtr<FJsonValue>> BlueprintRows;
	for (UBlueprint* BP : Blueprints)
	{
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("path"), BP->GetPathName());
		Row->SetStringField(TEXT("parent"), GetPathNameSafe(BP->ParentClass));
		TArray<UEdGraph*> Graphs;
		BP->GetAllGraphs(Graphs);
		TArray<TSharedPtr<FJsonValue>> GraphRows;
		for (UEdGraph* Graph : Graphs)
		{
			TSharedRef<FJsonObject> GraphRow = MakeShared<FJsonObject>();
			GraphRow->SetStringField(TEXT("name"), Graph->GetName());
			GraphRow->SetStringField(TEXT("class"), Graph->GetClass()->GetName());
			TArray<TSharedPtr<FJsonValue>> Nodes;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				TSharedRef<FJsonObject> NodeRow = MakeShared<FJsonObject>();
				NodeRow->SetStringField(TEXT("name"), Node->GetName());
				NodeRow->SetStringField(TEXT("class"), Node->GetClass()->GetName());
				NodeRow->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
				TArray<TSharedPtr<FJsonValue>> Pins;
				for (UEdGraphPin* Pin : Node->Pins)
				{
					TSharedRef<FJsonObject> PinRow = MakeShared<FJsonObject>();
					PinRow->SetStringField(TEXT("name"), Pin->PinName.ToString());
					PinRow->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
					PinRow->SetStringField(TEXT("default"), Pin->DefaultValue);
					PinRow->SetStringField(TEXT("object"), GetPathNameSafe(Pin->DefaultObject));
					PinRow->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"));
					TArray<TSharedPtr<FJsonValue>> Links;
					for (UEdGraphPin* Link : Pin->LinkedTo)
						Links.Add(MakeShared<FJsonValueString>(Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString()));
					PinRow->SetArrayField(TEXT("links"), Links);
					Pins.Add(MakeShared<FJsonValueObject>(PinRow));
				}
				NodeRow->SetArrayField(TEXT("pins"), Pins);
				Nodes.Add(MakeShared<FJsonValueObject>(NodeRow));
			}
			GraphRow->SetArrayField(TEXT("nodes"), Nodes);
			GraphRows.Add(MakeShared<FJsonValueObject>(GraphRow));
		}
		Row->SetArrayField(TEXT("graphs"), GraphRows);
		BlueprintRows.Add(MakeShared<FJsonValueObject>(Row));
	}
	Root->SetArrayField(TEXT("blueprints"), BlueprintRows);
	FString Result;
	FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Result));
	return Result;
}
