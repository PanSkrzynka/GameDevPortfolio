#pragma once

#include "CoreMinimal.h"

// Internal timing and geometry defaults. Designer-facing behavior remains in UPropertyResponseProfile.
namespace PropertyAIConstants
{
constexpr float SightMemorySeconds = 2.f;
constexpr float DefaultSightRadiusCm = 1400.f;
constexpr float DefaultLoseSightRadiusCm = 1600.f;
constexpr float DefaultVisionHalfAngleDegrees = 70.f;
constexpr float LoseSightMarginCm = 200.f;
constexpr float AwarenessIntervalSeconds = .2f;
constexpr float EvidenceReportIntervalSeconds = 2.f;
constexpr float ResponseIntervalSeconds = .15f;
constexpr float MaximumResponseSpeedCmPerSecond = 1200.f;
constexpr float ArrivalDistanceCm = 120.f;
constexpr float FocusDistanceCm = 1000.f;
constexpr float ReturnAcceptanceRadiusCm = 70.f;
constexpr float ChaseAcceptanceRadiusCm = 70.f;
constexpr float SearchAcceptanceRadiusCm = 65.f;
constexpr float PatrolAcceptanceRadiusCm = 65.f;
constexpr float RoomInspectionExtentScale = .6f;
// Golden-angle spacing distributes responders around the room perimeter.
constexpr float SearchSlotAngleRadians = 2.39996f;
constexpr float SearchSweepAngleOffsetRadians = .35f;
constexpr float SearchPerimeterMarginCm = 200.f;
const FVector SearchNavigationExtent(150, 150, 250);
constexpr float AssignedSearchUpdateSeconds = 4.f;
constexpr float SoloSearchUpdateSeconds = 1.5f;
constexpr float AssignedSearchDurationScale = 2.f;
constexpr float MinimumChaseBreakOffDistanceCm = 100.f;
constexpr float ChaseUpdateSeconds = .5f;
constexpr float MinimumPauseSeconds = .1f;
constexpr float QuestioningTimeoutSeconds = 20.f;
constexpr float QuestioningBreakOffDistanceCm = 400.f;
constexpr float ConfrontBreakOffDistanceScale = 1.5f;
constexpr float AttackIntervalSeconds = 1.f;
constexpr float AttackDamage = 15.f;
constexpr float ReturnTimeoutSeconds = 15.f;
constexpr float PriorOffenceWarningWeight = .5f;
constexpr float MinimumFollowRadiusCm = 10.f;
constexpr float FollowRadiusScale = .5f;
constexpr int32 CompensationPerTheft = 100;
constexpr int32 RepeatOffenceSurcharge = 50;
constexpr float QuestioningCooldownSeconds = 60.f;
constexpr float DefeatRecoverySeconds = 15.f;
constexpr float NoiseRetargetDistanceCm = 100.f;
constexpr float AlertPatrolSpeedScale = 1.25f;
constexpr float AlertPatrolWaitScale = .5f;
constexpr float PatrolUpdateSeconds = 1.f;
constexpr float MinimumFailedPatrolWaitSeconds = 1.f;
constexpr float LoadGraceSeconds = 1.f;
constexpr float MissingReportLifetimeSeconds = 90.f;
} // namespace PropertyAIConstants
