#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PropertyAssetBuilder.generated.h"

class UBlueprint;
class UStateTree;

UCLASS()
class NPCPROPERTYEDITOR_API UPropertyAssetBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static bool BuildNPCPolicyGraphs(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static bool BuildCrouchGraph(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Property Editor") static bool UpgradeAwarenessPresentation(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category="Property", meta=(WorldContext="WorldContextObject"))
	static bool RebuildNavigation(UObject* WorldContextObject, const TArray<FVector>& ValidationPoints);
	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static UStateTree* CreateResponseTree(const FString& AssetPath);

	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static bool BuildResponseGraph(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static bool AddNavigationBounds(UObject* WorldContextObject, FVector Center, FVector Extent);

	UFUNCTION(BlueprintCallable, Category="Property Editor")
	static int32 RemoveBlueprintComments(UBlueprint* Blueprint);
};
