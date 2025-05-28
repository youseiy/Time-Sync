// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RewindTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "RewindSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class REWIND_API URewindSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,Category="TimeSync|RewindSubsystem")
	void AddActor(AActor* InActor,URewindComponent* InComponent);
	UFUNCTION(BlueprintCallable,Category="TimeSync|RewindSubsystem")
	void RemoveActor(AActor* InActor);
	UFUNCTION(BlueprintCallable,Category="TimeSync|RewindSubsystem")
	bool IsReversing() const;
	UFUNCTION(BlueprintCallable,Category="TimeSync|RewindSubsystem")
	void SetRewindConfig(const FRewindConfig& InRewindConfig );
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void HandleForwardRecording(float DeltaTime, FActorFrameSnapshot& Snapshot);

	void HandleReversePlayback(float DeltaTime);

	void RemovePendingKillActorsOrRequested();

	void CalculateSnapshot(FRewindedActorFrameSnapshot& SnapshotOut, bool bIsCharacter);
	
	
	virtual void Tick(float DeltaTime) override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override {
		RETURN_QUICK_DECLARE_CYCLE_STAT(URewindSubsystem, STATGROUP_Tickables);
	}
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return Super::ShouldCreateSubsystem(Outer); };
	
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override{ return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;};

	static void SetSnapshotVariables(AActor* InActor, const FVector& SnapshotLocation, const FRotator& SnapshotRotation, const FVector& SnapshotLinearVelocity, const FVector& SnapshotAngularVelocity);

	static FPoseSnapshot InterpPoseSnapshotTo(const FPoseSnapshot& Current, const FPoseSnapshot& Target, float DeltaTime, float InterpSpeed);

	
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartReverse);
	UPROPERTY(BlueprintCallable)
	FStartReverse OnStartReverse;
	UFUNCTION(BlueprintCallable)
	void StartReverse();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndReverse);
	UPROPERTY(BlueprintCallable)
	FEndReverse OnEndReverse;
	UFUNCTION(BlueprintCallable)
	void EndReverse();
	
	bool bRewindingTime{ false };

	bool bIsExecutingThreadTask{ false };

	bool bShouldContinueRewinding{true};


	TArray<TWeakObjectPtr<AActor>> PendingRemoveActors;

	TSet<FRewindActor> ReverseActors;
	//TArray<FRewindActor> ReverseActors;
	
	TMap<TWeakObjectPtr<AActor>, FActorData> ActorsData;
	
	FRewindConfig RewindConfig;
};
