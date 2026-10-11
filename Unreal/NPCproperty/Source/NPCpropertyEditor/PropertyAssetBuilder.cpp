#include "PropertyAssetBuilder.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "Property/PropertyPlayerAnimInstance.h"
#include "Property/AI/PropertyStateTreeNodes.h"
#include "Property/Demo/PropertyWitness.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "AssetCompilingManager.h"
#include "UObject/UObjectGlobals.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"

namespace PropertyPolicyGraphs
{
    struct FGraph
    {
        UEdGraph* Graph;
        UK2Node_FunctionEntry* Entry = nullptr;
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
        FName Function;
        bool bValid = true;

        FGraph(UBlueprint* Blueprint, FName Name) : Function(Name)
        {
            Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
            FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Graph, false, APropertyWitness::StaticClass());
            const auto Nodes = Graph->Nodes;
            for (UEdGraphNode* Node : Nodes)
            {
                if (auto* Found = Cast<UK2Node_FunctionEntry>(Node)) Entry = Found;
                if (Cast<UK2Node_FunctionResult>(Node)) FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
            }
            check(Entry);
            Entry->NodePosX = 0;
            Entry->NodePosY = 0;
        }

        void Link(UEdGraphPin* From, UEdGraphPin* To)
        {
            bValid &= From && To && Schema->TryCreateConnection(From, To);
        }

        UK2Node_SwitchEnum* Switch(UEnum* Enum, UEdGraphPin* Exec, UEdGraphPin* Value, int32 X, int32 Y)
        {
            FGraphNodeCreator<UK2Node_SwitchEnum> Creator(*Graph);
            auto* Node = Creator.CreateNode();
            Node->SetEnum(Enum);
            Node->NodePosX = X;
            Node->NodePosY = Y;
            Creator.Finalize();
            Link(Exec, Node->GetExecPin());
            Link(Value, Node->GetSelectionPin());
            return Node;
        }

        UK2Node_IfThenElse* Branch(UEdGraphPin* Exec, FName Condition, int32 X, int32 Y)
        {
            FGraphNodeCreator<UK2Node_IfThenElse> Creator(*Graph);
            auto* Node = Creator.CreateNode();
            Node->NodePosX = X;
            Node->NodePosY = Y;
            Creator.Finalize();
            Link(Exec, Node->GetExecPin());
            Link(Entry->FindPinChecked(Condition), Node->GetConditionPin());
            return Node;
        }

        UK2Node_FunctionResult* Result(UEdGraphPin* Exec, int32 X, int32 Y)
        {
            FGraphNodeCreator<UK2Node_FunctionResult> Creator(*Graph);
            auto* Node = Creator.CreateNode();
            Node->FunctionReference.SetExternalMember(Function, APropertyWitness::StaticClass());
            Node->NodePosX = X;
            Node->NodePosY = Y;
            Creator.Finalize();
            Link(Exec, Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
            return Node;
        }

        void Cue(UEdGraphPin* Exec, EPropertyDialogueCue Value, int32 X, int32 Y)
        {
            auto* Node = Result(Exec, X, Y);
            Schema->TrySetDefaultValue(*Node->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue), StaticEnum<EPropertyDialogueCue>()->GetNameStringByValue(static_cast<int64>(Value)));
        }
    };

    UEdGraphPin* Case(UK2Node_SwitchEnum* Node, UEnum* Enum, int64 Value)
    {
        return Node->FindPinChecked(FName(*Enum->GetNameStringByValue(Value)), EGPD_Output);
    }
}

bool UPropertyAssetBuilder::BuildNPCPolicyGraphs(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->ParentClass->IsChildOf(APropertyWitness::StaticClass())) return false;
    using namespace PropertyPolicyGraphs;
    const auto Exists = [&](FName Name)
    {
        return Blueprint->FunctionGraphs.ContainsByPredicate([&](const UEdGraph* Graph) { return Graph->GetFName() == Name; });
    };
    bool bValid = true;
    auto* States = StaticEnum<EPropertyResponseState>();
    if (!Exists(GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectResponseCue)))
    {
        FGraph PolicyGraph(Blueprint, GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectResponseCue));
        auto* State = PolicyGraph.Switch(States, PolicyGraph.Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), PolicyGraph.Entry->FindPinChecked(TEXT("State")), 350, 0);
        PolicyGraph.Cue(Case(State, States, 0), EPropertyDialogueCue::RoutineStatus, 750, -400);
        PolicyGraph.Cue(Case(State, States, 1), EPropertyDialogueCue::WarningStatus, 750, -250);
        PolicyGraph.Cue(Case(State, States, 2), EPropertyDialogueCue::ChaseStatus, 750, -100);
        PolicyGraph.Cue(Case(State, States, 5), EPropertyDialogueCue::ReturnStatus, 750, 50);
        auto* Roles = StaticEnum<EPropertySearchRole>();
        auto* Role = PolicyGraph.Switch(Roles, Case(State, States, 3), PolicyGraph.Entry->FindPinChecked(TEXT("SearchAssignment")), 750, 300);
        PolicyGraph.Cue(Case(Role, Roles, 1), EPropertyDialogueCue::InspectRoomStatus, 1100, 300);
        PolicyGraph.Cue(Case(Role, Roles, 2), EPropertyDialogueCue::WatchExitStatus, 1100, 450);
        PolicyGraph.Cue(Case(Role, Roles, 3), EPropertyDialogueCue::SweepStatus, 1100, 600);
        auto* Suspect = PolicyGraph.Branch(Case(Role, Roles, 0), TEXT("bHasSuspect"), 1100, 800);
        PolicyGraph.Cue(Suspect->GetThenPin(), EPropertyDialogueCue::LostSuspectStatus, 1450, 800);
        PolicyGraph.Cue(Suspect->GetElsePin(), EPropertyDialogueCue::InvestigatingStatus, 1450, 950);
        auto* Question = PolicyGraph.Branch(Case(State, States, 4), TEXT("bQuestioning"), 750, 1200);
        PolicyGraph.Cue(Question->GetThenPin(), EPropertyDialogueCue::QuestioningStatus, 1100, 1200);
        auto* Crime = PolicyGraph.Branch(Question->GetElsePin(), TEXT("bHasCrime"), 1100, 1400);
        PolicyGraph.Cue(Crime->GetThenPin(), EPropertyDialogueCue::TheftConfrontStatus, 1450, 1400);
        PolicyGraph.Cue(Crime->GetElsePin(), EPropertyDialogueCue::TrespassConfrontStatus, 1450, 1550);
        bValid &= PolicyGraph.bValid;
    }
    if (!Exists(GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectResponseSpeed)))
    {
        FGraph PolicyGraph(Blueprint, GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectResponseSpeed));
        auto* State = PolicyGraph.Switch(States, PolicyGraph.Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), PolicyGraph.Entry->FindPinChecked(TEXT("State")), 350, 0);
        for (int32 Index = 0; Index < 6; ++Index)
        {
            auto* Return = PolicyGraph.Result(Case(State, States, Index), 750, Index * 180);
            PolicyGraph.Link(PolicyGraph.Entry->FindPinChecked(Index == 2 ? TEXT("ChaseSpeed") : TEXT("WalkSpeed")), Return->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue));
        }
        bValid &= PolicyGraph.bValid;
    }
    if (!Exists(GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectNoiseCue)))
    {
        FGraph PolicyGraph(Blueprint, GET_FUNCTION_NAME_CHECKED(APropertyWitness, SelectNoiseCue));
        auto* Kinds = StaticEnum<EPropertyNoiseKind>();
        auto* Kind = PolicyGraph.Switch(Kinds, PolicyGraph.Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), PolicyGraph.Entry->FindPinChecked(TEXT("Kind")), 350, 0);
        for (int32 Index = 0; Index < 5; ++Index)
            PolicyGraph.Cue(Case(Kind, Kinds, Index), Index == static_cast<int32>(EPropertyNoiseKind::Alarm) ? EPropertyDialogueCue::HearAlarm : EPropertyDialogueCue::HearNoise, 750, Index * 180);
        bValid &= PolicyGraph.bValid;
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();
    return bValid && Blueprint->Status != BS_Error;
}

UStateTree* UPropertyAssetBuilder::CreateResponseTree(const FString& AssetPath)
{
	const FString ObjectPath = AssetPath + TEXT(".") + FPackageName::GetLongPackageAssetName(AssetPath);
	if (FPackageName::DoesPackageExist(AssetPath)) return LoadObject<UStateTree>(nullptr, *ObjectPath);
	UPackage* Package = CreatePackage(*AssetPath);
	UStateTree* Tree = NewObject<UStateTree>(Package, *FPackageName::GetLongPackageAssetName(AssetPath), RF_Public | RF_Standalone | RF_Transactional);
	auto* Data = NewObject<UStateTreeEditorData>(Tree, TEXT("EditorData"), RF_Transactional);
	Tree->EditorData = Data;
	Data->Schema = NewObject<UStateTreeAIComponentSchema>(Data);
	UStateTreeState& Root = Data->AddRootState();
	UEnum* States = StaticEnum<EPropertyResponseState>();
	for (uint8 Index = 0; Index <= static_cast<uint8>(EPropertyResponseState::Return); ++Index)
	{
		const auto State = static_cast<EPropertyResponseState>(Index);
		UStateTreeState& Child = Root.AddChildState(*States->GetNameStringByValue(Index));
		Child.AddEnterCondition<FPropertyResponseCondition>().GetInstanceData().State = State;
		Child.AddTask<FPropertyResponseTask>().GetInstanceData().State = State;
		Child.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);
	}
	FStateTreeCompilerLog Log;
	FStateTreeCompiler Compiler(Log);
	if (!Compiler.Compile(*Tree))
	{
		Log.DumpToLog(LogTemp);
		return nullptr;
	}
	FAssetRegistryModule::AssetCreated(Tree);
	Tree->MarkPackageDirty();
	return Tree;
}

bool UPropertyAssetBuilder::RebuildNavigation(UObject* WorldContextObject, const TArray<FVector>& ValidationPoints)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	auto* Navigation = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!Navigation) return false;
	FlushAsyncLoading();
	FAssetCompilingManager::Get().FinishAllCompilation();
	Navigation->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock, UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) Navigation->OnNavigationBoundsUpdated(*It);
	Navigation->Build();
	auto* Data = Navigation->GetDefaultNavDataInstance(FNavigationSystem::Create);
	if (!Data) return false;
	Data->EnsureBuildCompletion();
	Data->MarkPackageDirty();
	for (const FVector& Point : ValidationPoints)
	{
		FNavLocation Projected;
		if (!Navigation->ProjectPointToNavigation(Point, Projected, FVector(150, 150, 250)))
		{
			UE_LOG(LogTemp, Error, TEXT("Navigation does not cover %s"), *Point.ToString());
			return false;
		}
	}
	return true;
}

bool UPropertyAssetBuilder::BuildResponseGraph(UBlueprint* Blueprint)
{
	if (!Blueprint || !Blueprint->ParentClass->IsChildOf(APropertyWitness::StaticClass())) return false;
	for (UEdGraph* Existing : Blueprint->UbergraphPages)
	{
		if (Existing->GetFName() == TEXT("ResponsePresentation")) return true;
	}
	UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, TEXT("ResponsePresentation"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);
	const auto* Schema = GetDefault<UEdGraphSchema_K2>();
	FGraphNodeCreator<UK2Node_Event> EventCreator(*Graph);
	auto* Event = EventCreator.CreateNode();
	Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(APropertyWitness, ReceiveResponseChanged), APropertyWitness::StaticClass());
	Event->bOverrideFunction = true;
	Event->NodePosX = 0;
	Event->NodePosY = 0;
	EventCreator.Finalize();
	FGraphNodeCreator<UK2Node_SwitchEnum> SwitchCreator(*Graph);
	auto* Switch = SwitchCreator.CreateNode();
	Switch->SetEnum(StaticEnum<EPropertyResponseState>());
	Switch->NodePosX = 320;
	Switch->NodePosY = 0;
	SwitchCreator.Finalize();
	bool bValid = Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Switch->GetExecPin());
	bValid &= Schema->TryCreateConnection(Event->FindPinChecked(TEXT("State")), Switch->GetSelectionPin());
	const TCHAR* Labels[] = {TEXT("On duty"), TEXT("Private property. Leave now!"), TEXT("Stop right there!"), TEXT("Where did you go?"), TEXT("That's enough. Leave now."), TEXT("Returning to my post")};
	const FColor Colors[] = {FColor(180, 230, 190), FColor(255, 190, 45), FColor(255, 70, 50), FColor(90, 180, 255), FColor(255, 120, 60), FColor(180, 180, 200)};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
	{
		FGraphNodeCreator<UK2Node_VariableGet> VariableCreator(*Graph);
		auto* Variable = VariableCreator.CreateNode();
		Variable->VariableReference.SetSelfMember(GET_MEMBER_NAME_CHECKED(APropertyWitness, StatusLabel));
		Variable->NodePosX = 600;
		Variable->NodePosY = Index * 260 + 120;
		VariableCreator.Finalize();
		FGraphNodeCreator<UK2Node_CallFunction> TextCreator(*Graph);
		auto* Text = TextCreator.CreateNode();
		Text->SetFromFunction(UTextRenderComponent::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UTextRenderComponent, K2_SetText)));
		Text->NodePosX = 800;
		Text->NodePosY = Index * 260;
		TextCreator.Finalize();
		FGraphNodeCreator<UK2Node_CallFunction> ColorCreator(*Graph);
		auto* Color = ColorCreator.CreateNode();
		Color->SetFromFunction(UTextRenderComponent::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UTextRenderComponent, SetTextRenderColor)));
		Color->NodePosX = 1150;
		Color->NodePosY = Index * 260;
		ColorCreator.Finalize();
		FGraphNodeCreator<UK2Node_CallFunction> LiteralCreator(*Graph);
		auto* Literal = LiteralCreator.CreateNode();
		Literal->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralText)));
		Literal->NodePosX = 440;
		Literal->NodePosY = Index * 260 + 190;
		LiteralCreator.Finalize();
		Schema->TrySetDefaultText(*Literal->FindPinChecked(TEXT("Value")), FText::FromString(Labels[Index]));
		bValid &= Schema->TryCreateConnection(Literal->GetReturnValuePin(), Text->FindPinChecked(TEXT("Value")));
		Schema->TrySetDefaultValue(*Color->FindPinChecked(TEXT("Value")), Colors[Index].ToString());
		bValid &= Schema->TryCreateConnection(Switch->FindPinChecked(*StaticEnum<EPropertyResponseState>()->GetNameStringByValue(Index)), Text->GetExecPin());
		bValid &= Schema->TryCreateConnection(Text->GetThenPin(), Color->GetExecPin());
		bValid &= Schema->TryCreateConnection(Variable->GetValuePin(), Text->FindPinChecked(UEdGraphSchema_K2::PN_Self));
		bValid &= Schema->TryCreateConnection(Variable->GetValuePin(), Color->FindPinChecked(UEdGraphSchema_K2::PN_Self));
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return bValid && Blueprint->Status != BS_Error;
}

bool UPropertyAssetBuilder::AddNavigationBounds(UObject* WorldContextObject, FVector Center, FVector Extent)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) return false;
	ANavMeshBoundsVolume* Volume = nullptr;
	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) { Volume = *It; break; }
	if (!Volume) Volume = World->SpawnActor<ANavMeshBoundsVolume>(Center, FRotator::ZeroRotator);
	Volume->Modify();
	auto* Builder = NewObject<UCubeBuilder>();
	Builder->X = Extent.X * 2.f;
	Builder->Y = Extent.Y * 2.f;
	Builder->Z = Extent.Z * 2.f;
	UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
	Volume->SetActorLabel(TEXT("Property navigation"));
	Volume->SetActorLocation(Center);
	if (auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)) Navigation->OnNavigationBoundsUpdated(Volume);
	return true;
}

int32 UPropertyAssetBuilder::RemoveBlueprintComments(UBlueprint* Blueprint)
{
	if (!Blueprint) return 0;
	int32 Removed = 0;
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		const auto Nodes = Graph->Nodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (Node->IsA<UEdGraphNode_Comment>())
			{
				FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
				++Removed;
			}
			else if (!Node->NodeComment.IsEmpty())
			{
				Node->NodeComment.Reset();
				Node->bCommentBubbleVisible = false;
				++Removed;
			}
		}
	}
	if (Removed > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		Blueprint->MarkPackageDirty();
	}
	return Removed;
}

bool UPropertyAssetBuilder::BuildCrouchGraph(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->ParentClass->IsChildOf(UPropertyPlayerAnimInstance::StaticClass())) return false;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        if (Graph->GetFName() != TEXT("AnimGraph")) continue;
        UAnimGraphNode_Root* Root = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node->GetFName() == TEXT("PropertyCrouchPelvis")) return true;
            if (auto* Candidate = Cast<UAnimGraphNode_Root>(Node)) Root = Candidate;
        }
        if (!Root) return false;
        UEdGraphPin* Input = Root->FindPin(TEXT("Result"));
        if (!Input || Input->LinkedTo.Num() != 1) return false;
        UEdGraphPin* Original = Input->LinkedTo[0];
        const auto* Schema = Graph->GetSchema();
        FGraphNodeCreator<UAnimGraphNode_LocalToComponentSpace> LocalCreator(*Graph);
        auto* Local = LocalCreator.CreateNode();
        LocalCreator.Finalize();
        FGraphNodeCreator<UAnimGraphNode_ModifyBone> PelvisCreator(*Graph);
        auto* Pelvis = PelvisCreator.CreateNode();
        Pelvis->Rename(TEXT("PropertyCrouchPelvis"));
        Pelvis->Node.BoneToModify.BoneName = TEXT("pelvis");
        Pelvis->Node.Translation = FVector(0, 0, -38);
        Pelvis->Node.TranslationMode = BMM_Additive;
        Pelvis->Node.TranslationSpace = BCS_ComponentSpace;
        PelvisCreator.Finalize();
        FGraphNodeCreator<UK2Node_VariableGet> AlphaCreator(*Graph);
        auto* Alpha = AlphaCreator.CreateNode();
        Alpha->VariableReference.SetSelfMember(TEXT("CrouchAlpha"));
        AlphaCreator.Finalize();
        auto Connect = [&](UEdGraphPin* A, UEdGraphPin* B) { return A && B && Schema->TryCreateConnection(A, B); };
        Input->BreakAllPinLinks();
        bool Success = Connect(Original, Local->FindPin(TEXT("LocalPose")));
        Success &= Connect(Local->FindPin(TEXT("ComponentPose")), Pelvis->FindPin(TEXT("ComponentPose"), EGPD_Input));
        Success &= Connect(Alpha->GetValuePin(), Pelvis->FindPin(TEXT("Alpha")));
        UEdGraphPin* Previous = Pelvis->FindPin(TEXT("Pose"), EGPD_Output);
        int32 X = Root->NodePosX;
        Local->NodePosX = X; Local->NodePosY = Root->NodePosY;
        Pelvis->NodePosX = X + 220; Pelvis->NodePosY = Root->NodePosY;
        Alpha->NodePosX = X + 220; Alpha->NodePosY = Root->NodePosY + 300;
        for (const FName Foot : {FName(TEXT("foot_l")), FName(TEXT("foot_r"))})
        {
            FGraphNodeCreator<UAnimGraphNode_TwoBoneIK> IKCreator(*Graph);
            auto* IK = IKCreator.CreateNode();
            IK->Node.IKBone.BoneName = Foot;
            IK->Node.EffectorLocationSpace = BCS_BoneSpace;
            IK->Node.EffectorTarget = FBoneSocketTarget(FName(*(TEXT("ik_") + Foot.ToString())));
            IK->Node.JointTargetLocationSpace = BCS_ComponentSpace;
            IK->Node.JointTargetLocation = FVector(Foot == TEXT("foot_l") ? -20 : 20, 100, 50);
            IK->Node.bMaintainEffectorRelRot = true;
            IKCreator.Finalize();
            Success &= Connect(Previous, IK->FindPin(TEXT("ComponentPose"), EGPD_Input));
            Success &= Connect(Alpha->GetValuePin(), IK->FindPin(TEXT("Alpha")));
            Previous = IK->FindPin(TEXT("Pose"), EGPD_Output);
            X += 450;
            IK->NodePosX = X; IK->NodePosY = Root->NodePosY;
        }
        FGraphNodeCreator<UAnimGraphNode_ComponentToLocalSpace> ComponentCreator(*Graph);
        auto* Component = ComponentCreator.CreateNode();
        ComponentCreator.Finalize();
        Success &= Connect(Previous, Component->FindPin(TEXT("ComponentPose"), EGPD_Input));
        Success &= Connect(Component->FindPin(TEXT("Pose"), EGPD_Output), Input);
        Component->NodePosX = X + 300; Component->NodePosY = Root->NodePosY;
        Root->NodePosX = X + 550;
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        return Success && Blueprint->Status != BS_Error;
    }
    return false;
}


bool UPropertyAssetBuilder::UpgradeAwarenessPresentation(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->ParentClass->IsChildOf(APropertyWitness::StaticClass())) return false;
    const TCHAR* Labels[] = {TEXT("On duty"), TEXT("Private property. Leave now!"), TEXT("Stop right there!"), TEXT("Where did you go?"), TEXT("That's enough. Leave now."), TEXT("Returning to my post")};
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetFName() != TEXT("ResponsePresentation")) continue;
        const auto Nodes = Graph->Nodes;
        for (UEdGraphNode* Node : Nodes)
        {
            auto* Literal = Cast<UK2Node_CallFunction>(Node);
            if (!Literal || Literal->GetFunctionName() != TEXT("MakeLiteralText")) continue;
            const auto* Value = Literal->FindPin(TEXT("Value"));
            if (!Value) continue;
            int32 State = INDEX_NONE;
            for (int32 Index = 0; Index < 6; ++Index) if (Value->DefaultTextValue.ToString() == Labels[Index]) State = Index;
            if (State == INDEX_NONE) continue;
            FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
            auto* Description = Creator.CreateNode();
            Description->SetFromFunction(APropertyWitness::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(APropertyWitness, GetResponseDescription)));
            Description->NodePosX = Literal->NodePosX;
            Description->NodePosY = Literal->NodePosY;
            Creator.Finalize();
            const auto* Schema = Graph->GetSchema();
            Schema->TrySetDefaultValue(*Description->FindPinChecked(TEXT("State")), StaticEnum<EPropertyResponseState>()->GetNameStringByValue(State));
            const auto Links = Literal->GetReturnValuePin()->LinkedTo;
            for (auto* Link : Links)
            {
                Link->BreakLinkTo(Literal->GetReturnValuePin());
                if (!Schema->TryCreateConnection(Description->GetReturnValuePin(), Link)) return false;
            }
            FBlueprintEditorUtils::RemoveNode(Blueprint, Literal, true);
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        return Blueprint->Status != BS_Error;
    }
    return false;
}

