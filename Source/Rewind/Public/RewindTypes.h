#pragma once

#include "CoreMinimal.h"
#include "RewindComponent.h"
#include "Containers/List.h"
#include "RewindTypes.generated.h"




struct FActorFrameSnapshot 
{
	FActorFrameSnapshot() = default;

	FActorFrameSnapshot(
		const FVector& InLocation,
		const FRotator& InRotation,
		const FVector& InLinearVelocity,
		const FVector& InAngularVelocity,
		float InDeltaTime
	)
		: Location(InLocation)
		, Rotation(InRotation)
		, LinearVelocity(InLinearVelocity)
		, AngularVelocity(InAngularVelocity)
		, DeltaTime(InDeltaTime)
	{}

	FVector Location{FVector::ZeroVector};
	FRotator Rotation{FRotator::ZeroRotator};
	FVector LinearVelocity{FVector::ZeroVector};
	FVector AngularVelocity{FVector::ZeroVector};
	
	float DeltaTime = 0.f;

	//todo: find a good place to put this so that every actor doesnt have this data
	FPoseSnapshot PoseSnapshot;
};

struct FRewindedActorFrameSnapshot
{
	FRewindedActorFrameSnapshot() = default;

	FRewindedActorFrameSnapshot(
		const FVector& InLocation,
		const FRotator& InRotation,
		const FVector& InLinearVelocity,
		const FVector& InAngularVelocity,
		const FPoseSnapshot& InPoseSnapshot
	)
		: Location(InLocation)
		, Rotation(InRotation)
		, LinearVelocity(InLinearVelocity)
		, AngularVelocity(InAngularVelocity)
		, PoseSnapshot(InPoseSnapshot)
	{}	

	FVector Location{FVector::ZeroVector};
	FRotator Rotation{FRotator::ZeroRotator};
	FVector LinearVelocity{FVector::ZeroVector};
	FVector AngularVelocity{FVector::ZeroVector};
	FPoseSnapshot PoseSnapshot;
};

/*struct FPoseableActorFrameSnapshot : public FActorFrameSnapshot
{
	FPoseableActorFrameSnapshot() = default;

	FPoseableActorFrameSnapshot(
		const FVector& InLocation,
		const FRotator& InRotation,
		const FVector& InLinearVelocity,
		const FVector& InAngularVelocity,
		float InDeltaTime,
		const FPoseSnapshot& InPoseSnapshot,
		bool bInIsPlayerPawn = false
	)
		: FActorFrameSnapshot(InLocation, InRotation, InLinearVelocity, InAngularVelocity, InDeltaTime, bInIsPlayerPawn)
		, PoseSnapshot(InPoseSnapshot)
	{}

	FPoseSnapshot PoseSnapshot;
};*/

struct FActorData {
	FActorData() = default;

	FActorData(const FActorData& Other)
		: bReversingTime(Other.bReversingTime),
		  bOutOfData(Other.bOutOfData),
		  RunningTime(Other.RunningTime),
		  ReverseRunningTime(Other.ReverseRunningTime),
		  RecordedTime(Other.RecordedTime) 
	{
		
	}

	float LeftRunningTime{};
	float RightRunningTime{};
	bool bReversingTime{ false };
	bool bOutOfData;
	float RunningTime;
	float ReverseRunningTime;
	float RecordedTime;
	TDoubleLinkedList<FActorFrameSnapshot> StoredFrames;
};



USTRUCT(BlueprintType)
struct FRewindConfig
{
	GENERATED_BODY()

	FRewindConfig() = default;

	FRewindConfig(float InRewindSpeed, float InRecordedTime, UCurveFloat* InRewindCurve) : RecordedTime(InRecordedTime), RewindSpeed(InRewindSpeed), RewindCurve(InRewindCurve){};
	
	FRewindConfig(float InRewindSpeed, float InRecordedTime): RecordedTime(InRecordedTime), RewindSpeed(InRewindSpeed){}

	//Calculate Interp Variables outside the GameThread. Worth only if you are going to use rewind with a lot of actors. Profile to discover.
	bool bUseMultiThreading{false};
	//Recorded Time in Seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecordedTime{15.f};
	//Default Speed. If the Curve is set, this will be ignored
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RewindSpeed{1.f};
	//If the average frames remaining of all actors pass the MinAvgThreshold, the rewind will end 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinAvgThreshold = 3.f;
	//Optional Curve for speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> RewindCurve;

	bool IsCurveSet() const
	{
		if (RewindCurve)
		{
			return true;
		}
		return false;
	}
	
};


struct FRewindActor
{
	FRewindActor() = delete;

	FRewindActor(AActor* InActor, URewindComponent* InRewindComponent)
		: InActor(InActor), InRewindComponent(InRewindComponent) {}

	TWeakObjectPtr<AActor> InActor;
	TWeakObjectPtr<URewindComponent> InRewindComponent;

	bool IsValid() const { return InActor.IsValid() && InRewindComponent.IsValid(); }

	FORCEINLINE bool operator==(const FRewindActor& Other) const
	{
		return InActor == Other.InActor && InRewindComponent == Other.InRewindComponent;
	}
};

// Add a custom `TSet` comparison function:
FORCEINLINE uint32 GetTypeHash(const FRewindActor& Actor)
{
	return GetTypeHash(Actor.InActor);
}

