// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RewindComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class REWIND_API URewindComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class URewindSubsystem;
public:
	URewindComponent();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartReverseTime);
	UPROPERTY(BlueprintAssignable)
	FOnStartReverseTime OnStartReverseTime;

	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndReverseTime);
	UPROPERTY(BlueprintAssignable)
	FOnEndReverseTime OnEndReverseTime;

	UFUNCTION(BlueprintCallable,BlueprintPure,meta=(BlueprintThreadSafe))
	bool IsReversingTime() const;
	
	UFUNCTION(BlueprintCallable,BlueprintPure,meta=(BlueprintThreadSafe))
	FPoseSnapshot TryGetPose();
	
protected:
	virtual void BeginPlay() override;
private:
	void AddToRewind();
	void RemoveFromRewind();
	
	FPoseSnapshot TargetPose;
	
	bool bReversingTime{false};
	
	
};
