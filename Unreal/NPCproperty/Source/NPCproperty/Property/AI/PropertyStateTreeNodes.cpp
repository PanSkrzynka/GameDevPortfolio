#include "Property/AI/PropertyStateTreeNodes.h"
#include "Property/AI/PropertyAIController.h"
#include "StateTreeExecutionContext.h"

bool FPropertyResponseCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	UObject* Owner = Context.GetOwner();
	const auto* Controller = Cast<APropertyAIController>(Owner);
	return Controller && Controller->DesiredResponse == Context.GetInstanceData(*this).State;
}

EStateTreeRunStatus FPropertyResponseTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UObject* Owner = Context.GetOwner();
	if (auto* Controller = Cast<APropertyAIController>(Owner))
	{
		Controller->EnterResponse(Context.GetInstanceData(*this).State);
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FPropertyResponseTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	UObject* Owner = Context.GetOwner();
	if (auto* Controller = Cast<APropertyAIController>(Owner))
	{
		Controller->AdvanceResponse(DeltaTime);
		return Controller->DesiredResponse == Context.GetInstanceData(*this).State ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Failed;
}
