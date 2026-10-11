#include "CreateVehicleUICommandlet.h"
#include "HUD/VehicleTelemetryWidget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

namespace
{
const FString Folder = TEXT("/Game/VehicleTemplate/Blueprints/UI/");
UWidgetBlueprint* Create(const TCHAR* Name, UClass* Parent)
{
    UPackage* Package = CreatePackage(*(Folder + Name));
    return CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package, Name, BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
}
bool Save(UWidgetBlueprint* BP)
{
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) { return false; }
    FAssetRegistryModule::AssetCreated(BP);
    BP->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(BP->GetOutermost(), BP, *FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
void Bind(UWidgetBlueprint* BP, UWidget* Widget, FName Target, FName Source)
{
    FDelegateEditorBinding Binding;
    Binding.ObjectName = Widget->GetName();
    Binding.PropertyName = Target;
    Binding.SourceProperty = Source;
    Binding.Kind = EBindingKind::Property;
    FProperty* Property = FindFProperty<FProperty>(BP->ParentClass, Source);
    check(Property);
    Binding.SourcePath = FEditorPropertyPath(TArray<FFieldVariant>{ FFieldVariant(Property) });
    BP->Bindings.Add(Binding);
}
UCanvasPanelSlot* Place(UCanvasPanel* Parent, UWidget* Child, float X, float Y, float W, float H)
{
    auto* Slot = Parent->AddChildToCanvas(Child);
    Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H));
    return Slot;
}
UCanvasPanel* Panel(UWidgetBlueprint* BP, UCanvasPanel* Root, const TCHAR* Name, float W, float H, bool Right = false)
{
    auto* Border = BP->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name));
    Border->SetBrushColor(FLinearColor(.02f,.02f,.03f,.82f));
    Border->SetPadding(FMargin(0));
    auto* Slot = Place(Root, Border, Right ? -24 : 24, Right ? 24 : -24, W, H);
    Slot->SetAnchors(FAnchors(Right ? 1.f : 0.f, Right ? 0.f : 1.f));
    Slot->SetAlignment(FVector2D(Right ? 1.f : 0.f, Right ? 0.f : 1.f));
    Bind(BP, Border, TEXT("Visibility"), FName(*(FString(Name)+TEXT("Visibility"))));
    auto* Canvas = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(*(FString(Name)+TEXT("Content"))));
    Border->SetContent(Canvas);
    return Canvas;
}
void Text(UWidgetBlueprint* BP, UCanvasPanel* Parent, const TCHAR* Name, const TCHAR* Label, float X, float Y, float W, float H, int32 Size = 16, const TCHAR* Property = nullptr)
{
    auto* Widget = BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
    Widget->SetText(FText::FromString(Label));
    auto Font = Widget->GetFont(); Font.Size = Size; Widget->SetFont(Font);
    Widget->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.92f,.95f)));
    Place(Parent, Widget, X,Y,W,H);
    if (Property) Bind(BP, Widget, TEXT("Text"), Property);
}
void Bar(UWidgetBlueprint* BP, UCanvasPanel* Parent, const TCHAR* Name, float X, float Y, float W, FLinearColor Color, const TCHAR* Property)
{
    auto* Widget = BP->WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(Name));
    Widget->SetFillColorAndOpacity(Color);
    Place(Parent, Widget, X,Y,W,12);
    Bind(BP, Widget, TEXT("Percent"), Property);
}
UVehicleTelemetryPlot* Plot(UWidgetBlueprint* BP, UWidgetBlueprint* PlotBP, UCanvasPanel* Parent, const TCHAR* Name, float X, float Y, float W, float H, EVehicleTelemetryChannel Channel, int32 Index, FLinearColor Color)
{
    auto* Widget = BP->WidgetTree->ConstructWidget<UVehicleTelemetryPlot>(PlotBP->GeneratedClass.Get(), FName(Name));
    Widget->Channel = Channel; Widget->HUDGraphIndex = Index; Widget->TraceColor = Color;
    Place(Parent, Widget, X,Y,W,H);
    return Widget;
}
void AddPaintGraph(UWidgetBlueprint* BP)
{
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("OnPaint"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(BP, Graph, false, UUserWidget::StaticClass());
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes) {
        if (auto* E = Cast<UK2Node_FunctionEntry>(Node)) Entry = E;
        if (auto* R = Cast<UK2Node_FunctionResult>(Node)) Result = R;
    }
    check(Entry);
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    auto Call = [Graph](UClass* Class, FName Function, int32 X, int32 Y) {
        auto* Node = NewObject<UK2Node_CallFunction>(Graph);
        Node->SetFromFunction(Class->FindFunctionByName(Function));
        Graph->AddNode(Node); Node->CreateNewGuid(); Node->PostPlacedNewNode(); Node->AllocateDefaultPins(); Node->NodePosX=X; Node->NodePosY=Y;
        return Node;
    };
    auto* Points = Call(UVehicleTelemetryPlot::StaticClass(), TEXT("GetPlotPoints"), 240, 160);
    auto* Draw = Call(UWidgetBlueprintLibrary::StaticClass(), TEXT("DrawLines"), 550, 0);
    auto* Color = NewObject<UK2Node_VariableGet>(Graph);
    Color->VariableReference.SetSelfMember(TEXT("TraceColor"));
    Graph->AddNode(Color); Color->CreateNewGuid(); Color->PostPlacedNewNode(); Color->AllocateDefaultPins(); Color->NodePosX=250; Color->NodePosY=330;
    auto Link = [Schema](UEdGraphPin* A, UEdGraphPin* B) { check(A && B); check(Schema->TryCreateConnection(A,B)); };
    Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then)->BreakAllPinLinks();
    Link(Entry->FindPinChecked(UEdGraphSchema_K2::PN_Then), Draw->GetExecPin());
    Link(Entry->FindPinChecked(TEXT("Context")), Points->FindPinChecked(TEXT("Context")));
    Link(Entry->FindPinChecked(TEXT("Context")), Draw->FindPinChecked(TEXT("Context")));
    Link(Points->GetReturnValuePin(), Draw->FindPinChecked(TEXT("Points")));
    Link(Color->GetValuePin(), Draw->FindPinChecked(TEXT("Tint")));
    Schema->TrySetDefaultValue(*Draw->FindPinChecked(TEXT("Thickness")), TEXT("1.5"));
    if (Result) Link(Draw->GetThenPin(), Result->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
}
}

UCreateVehicleUICommandlet::UCreateVehicleUICommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}
int32 UCreateVehicleUICommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(Folder+TEXT("WBP_VehicleTelemetry")) || FPackageName::DoesPackageExist(Folder+TEXT("WBP_TelemetryPlot")))
    {
        UE_LOG(LogTemp, Error, TEXT("Telemetry Widget Blueprints already exist; edit them in the Designer."));
        return 1;
    }
    auto* PlotBP = Create(TEXT("WBP_TelemetryPlot"), UVehicleTelemetryPlot::StaticClass());
    AddPaintGraph(PlotBP);
    if (!Save(PlotBP)) return 1;
    auto* BP = Create(TEXT("WBP_VehicleTelemetry"), UVehicleTelemetryWidget::StaticClass());
    auto* Root = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TelemetryRoot"));
    BP->WidgetTree->RootWidget = Root;
    Root->SetVisibility(ESlateVisibility::HitTestInvisible);
    const FLinearColor Blue(.2f,.75f,1.f), Green(.3f,.9f,.4f), Red(1.f,.25f,.2f), Amber(1.f,.65f,.1f);
    auto* Core = Panel(BP,Root,TEXT("Core"),650,350);
    Text(BP,Core,TEXT("Speed"),TEXT("0 km/h"),16,12,230,55,32,TEXT("SpeedText"));
    Text(BP,Core,TEXT("Gear"),TEXT("N"),16,65,70,70,44,TEXT("GearText"));
    Text(BP,Core,TEXT("RPM"),TEXT("rpm"),95,86,145,30,18,TEXT("RPMText"));
    Bar(BP,Core,TEXT("RPMBar"),16,144,218,Blue,TEXT("RPMFraction"));
    Text(BP,Core,TEXT("Lap"),TEXT("LAP"),260,12,375,28,16,TEXT("LapText"));
    Text(BP,Core,TEXT("ThrottleLabel"),TEXT("THROTTLE"),260,48,150,22,12);
    Bar(BP,Core,TEXT("ThrottleBar"),260,72,370,Green,TEXT("Throttle"));
    Text(BP,Core,TEXT("BrakeLabel"),TEXT("BRAKE"),260,92,150,22,12);
    Bar(BP,Core,TEXT("BrakeBar"),260,116,370,Red,TEXT("Brake"));
    Text(BP,Core,TEXT("SteeringLabel"),TEXT("STEERING  L / R"),260,136,210,22,12);
    Bar(BP,Core,TEXT("SteeringBar"),260,160,370,Blue,TEXT("Steering"));
    Text(BP,Core,TEXT("Assists"),TEXT("ASSISTS"),16,184,610,28,14,TEXT("AssistsText"));
    Text(BP,Core,TEXT("Dynamics"),TEXT("DYNAMICS"),16,216,610,60,15,TEXT("DynamicsText"));
    Text(BP,Core,TEXT("Surface"),TEXT("SURFACE"),16,285,220,26,14,TEXT("SurfaceText"));
    Plot(BP,PlotBP,Core,TEXT("ThrottleHistory"),260,285,370,15,EVehicleTelemetryChannel::Throttle,-1,Green);
    Plot(BP,PlotBP,Core,TEXT("BrakeHistory"),260,305,370,15,EVehicleTelemetryChannel::Brake,-1,Red);
    Plot(BP,PlotBP,Core,TEXT("SteeringHistory"),260,325,370,15,EVehicleTelemetryChannel::Steering,-1,Blue);
    auto* Wheels = Panel(BP,Root,TEXT("Wheels"),650,430);
    Text(BP,Wheels,TEXT("WheelsTitle"),TEXT("WHEELS  /  Load - Slip ratio - Slip angle"),16,12,620,30,18);
    for (int32 i=0; i<4; ++i) {
        FString N=FString::Printf(TEXT("Wheel%d"),i);
        float Y=52+i*90;
        Text(BP,Wheels,*N,TEXT("WHEEL"),16,Y,610,48,14,*(N+TEXT("Text")));
        Bar(BP,Wheels,*(N+TEXT("LoadBar")),16,Y+49,195,Green,*(N+TEXT("Load")));
        Bar(BP,Wheels,*(N+TEXT("SlipBar")),220,Y+49,195,Amber,*(N+TEXT("Slip")));
        Bar(BP,Wheels,*(N+TEXT("AngleBar")),424,Y+49,195,Blue,*(N+TEXT("Angle")));
    }
    auto* Balance = Panel(BP,Root,TEXT("Balance"),650,420);
    Text(BP,Balance,TEXT("BalanceTitle"),TEXT("BALANCE"),16,12,600,28,18);
    Text(BP,Balance,TEXT("BalanceData"),TEXT("BALANCE"),16,52,375,125,18,TEXT("BalanceText"));
    Text(BP,Balance,TEXT("FrontLabel"),TEXT("FRONT AXLE LOAD"),16,282,300,24,12);
    Bar(BP,Balance,TEXT("FrontLoadBar"),16,308,610,Blue,TEXT("FrontLoad"));
    Text(BP,Balance,TEXT("LeftLabel"),TEXT("LEFT SIDE LOAD"),16,332,300,24,12);
    Bar(BP,Balance,TEXT("LeftLoadBar"),16,358,610,Amber,TEXT("LeftLoad"));
    Text(BP,Balance,TEXT("BalanceAssists"),TEXT("ASSISTS"),16,384,610,28,14,TEXT("AssistsText"));
    for (int32 Ring = 0; Ring < 2; ++Ring)
    {
        auto* Circle = Plot(BP,PlotBP,Balance,*FString::Printf(TEXT("GCircle%d"),Ring),400,48,200,200,EVehicleTelemetryChannel::CombinedG,-1,FLinearColor(.4f,.45f,.5f));
        const float Radius = Ring == 0 ? .5f : .25f;
        for (int32 i=0;i<=64;++i) { float Angle = i*2.f*PI/64.f; Circle->StaticPoints.Add(FVector2D(.5f+Radius*FMath::Cos(Angle),.5f+Radius*FMath::Sin(Angle))); }
    }
    auto* Trail = Plot(BP,PlotBP,Balance,TEXT("GForceTrail"),400,48,200,200,EVehicleTelemetryChannel::CombinedG,-1,Blue);
    Trail->bTractionTrail = true;
    Text(BP,Balance,TEXT("GCircleLabel"),TEXT("TRACTION  /  1g - 2g"),400,254,235,22,12);
    auto* Graphs = Panel(BP,Root,TEXT("Graphs"),650,470);
    Text(BP,Graphs,TEXT("GraphsTitle"),TEXT("TRACES"),16,12,610,28,18,TEXT("GraphSummaryText"));
    const TCHAR* Labels[] = { TEXT("PRIMARY"), TEXT("SECONDARY"), TEXT("YAW ERROR"), TEXT("TERTIARY") };
    FLinearColor Colors[] = { Blue, Amber, Red, Green };
    for (int32 i=0; i<4; ++i) {
        FString N=FString::Printf(TEXT("Graph%d"),i); float Y=50+i*102;
        Text(BP,Graphs,*(N+TEXT("Label")),Labels[i],16,Y,610,24,12,*(N+TEXT("Text")));
        Plot(BP,PlotBP,Graphs,*N,16,Y+28,610,66,EVehicleTelemetryChannel::Speed,i,Colors[i]);
    }
    auto* Standings = Panel(BP,Root,TEXT("Standings"),500,350,true);
    Text(BP,Standings,TEXT("StandingsRows"),TEXT("RACE"),16,12,468,326,16,TEXT("StandingsText"));
    return Save(BP) ? 0 : 1;
}
