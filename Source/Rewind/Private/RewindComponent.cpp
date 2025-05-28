// Fill out your copyright notice in the Description page of Project Settings.


#include "RewindComponent.h"
#include "Rewind.h"
#include "RewindSubsystem.h"
#include "Logging/StructuredLog.h"


// Sets default values for this component's properties
URewindComponent::URewindComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


bool URewindComponent::IsReversingTime() const
{
	return bReversingTime;
}

FPoseSnapshot URewindComponent::TryGetPose()
{
	if (!TargetPose.bIsValid)
	{
		UE_LOGFMT(LogRewind,Warning,"Target Pose is not valid.");
	}
	return TargetPose;
}
void URewindComponent::BeginPlay()
{
	Super::BeginPlay();

	AddToRewind();
}

void URewindComponent::AddToRewind()
{
	auto Subsystem{GetWorld()->GetSubsystem<URewindSubsystem>()};

	if (!IsValid(Subsystem))
	{
		UE_LOGFMT(LogRewind,Warning,"RewindSubsystem is not valid.");
		return;
	}
	Subsystem->AddActor(GetOwner(),this);
}