#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "Property/AI/PropertyResponseProfile.h"
#include "PropertyStateTreeNodes.generated.h"

USTRUCT()
struct NPCPROPERTY_API FPropertyResponseNodeData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Parameter")
	EPropertyResponseState State = EPropertyResponseState::Routine;
};

USTRUCT(meta=(DisplayName="Property Response Requested", Category="Property"))
struct NPCPROPERTY_API FPropertyResponseCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPropertyResponseNodeData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta=(DisplayName="Execute Property Response", Category="Property"))
struct NPCPROPERTY_API FPropertyResponseTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPropertyResponseNodeData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
