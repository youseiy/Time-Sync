// Fill out your copyright notice in the Description page of Project Settings.


#include "RewindSubsystem.h"

#include "Rewind.h"
#include "RewindDeveloperSettings.h"
#include "Algo/RemoveIf.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "Logging/StructuredLog.h"

void URewindSubsystem::AddActor(AActor* InActor,URewindComponent* InComponent)
{
	ReverseActors.Emplace(FRewindActor{InActor,InComponent});
}

void URewindSubsystem::RemoveActor(AActor* InActor)
{
	PendingRemoveActors.Emplace(InActor);
}

bool URewindSubsystem::IsReversing() const
{
	return bRewindingTime;
}

void URewindSubsystem::SetRewindConfig(const FRewindConfig& InRewindConfig)
{
	RewindConfig = InRewindConfig;
}

void URewindSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnStartReverse.AddDynamic(this, &ThisClass::StartReverse);
	OnEndReverse.AddDynamic(this, &ThisClass::EndReverse);

	auto Settings{GetDefault<URewindDeveloperSettings>()};
	
	auto CurveToLoad{GetDefault<URewindDeveloperSettings>()->GetRewindCurveFloat()};

	if (!CurveToLoad)
	{
		RewindConfig=FRewindConfig{Settings->GetRewindSpeed(),Settings->GetRewindSpeed()};
		return;
	}
	
	FStreamableManager StreamableManager;

	StreamableManager.RequestAsyncLoad(CurveToLoad.ToSoftObjectPath(),[this]()
	{
		auto Settings{GetDefault<URewindDeveloperSettings>()};
		
		RewindConfig=FRewindConfig{Settings->GetRewindSpeed(),Settings->GetRewindSpeed(),Settings->GetRewindCurveFloat().Get()};
	});
	
}



void URewindSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TRACE_CPUPROFILER_EVENT_SCOPE(URewindSubsystem_Tick);

	
    
	if (ReverseActors.IsEmpty()) return;

	// ----- STEP 1: Create Snapshot (reuse variable) -----
	FActorFrameSnapshot Snapshot{};
	
	RemovePendingKillActorsOrRequested();
	
	if (!bRewindingTime)
	{
		// -----  Handle Forward Recording -----
		HandleForwardRecording(DeltaTime, Snapshot);
	}
	else
	{
		// ----- OR : Handle Reverse Playback -----
		HandleReversePlayback(DeltaTime);
	}
}

void URewindSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URewindSubsystem::SetSnapshotVariables(AActor* InActor, const FVector& SnapshotLocation,
	const FRotator& SnapshotRotation, const FVector& SnapshotLinearVelocity, const FVector& SnapshotAngularVelocity)
{
	InActor->SetActorLocation(SnapshotLocation);
	InActor->SetActorRotation(SnapshotRotation);

	if (auto* MeshRoot = Cast<UPrimitiveComponent>(InActor->GetRootComponent()))
	{
		MeshRoot->SetPhysicsLinearVelocity(SnapshotLinearVelocity);
		MeshRoot->SetPhysicsAngularVelocityInRadians(SnapshotAngularVelocity);
	}
	
}

FPoseSnapshot URewindSubsystem::InterpPoseSnapshotTo(const FPoseSnapshot& Current, const FPoseSnapshot& Target,
	float DeltaTime, float InterpSpeed)
{
	FPoseSnapshot Result;

	// Sanity checks
	if (!Current.bIsValid || !Target.bIsValid)
	{
		return Result; // bIsValid false by default
	}

	if (Current.BoneNames.Num() != Target.BoneNames.Num() || Current.LocalTransforms.Num() != Target.LocalTransforms.Num())
	{
		return Result;
	}

	const int32 NumBones = Current.BoneNames.Num();
	for (int32 i = 0; i < NumBones; ++i)
	{
		if (Current.BoneNames[i] != Target.BoneNames[i])
		{
			return Result;
		}

		const FTransform& CurrentTransform = Current.LocalTransforms[i];
		const FTransform& TargetTransform = Target.LocalTransforms[i];

		FTransform InterpedTransform;
		InterpedTransform.SetLocation(FMath::Lerp(CurrentTransform.GetLocation(), TargetTransform.GetLocation(), DeltaTime));
		InterpedTransform.SetRotation(FQuat::Slerp(CurrentTransform.GetRotation(), TargetTransform.GetRotation(), DeltaTime));
		InterpedTransform.SetScale3D(FMath::Lerp(CurrentTransform.GetScale3D(), TargetTransform.GetScale3D(), DeltaTime));

		Result.LocalTransforms.Add(InterpedTransform);
		Result.BoneNames.Add(Current.BoneNames[i]);
	}

	Result.SkeletalMeshName = Current.SkeletalMeshName; 
	Result.SnapshotName = NAME_None; 
	Result.bIsValid = true;

	return Result;
}

void URewindSubsystem::HandleForwardRecording(float DeltaTime,FActorFrameSnapshot& Snapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(URewindSubsystem_Recording);
	
	auto RecordedTimeSeconds{GetDefault<URewindDeveloperSettings>()->GetRecordedTimeSeconds()};
	
	  for (auto& Actor : ReverseActors)
    {
        if (!Actor.IsValid()) continue;

        auto& Data = ActorsData.FindOrAdd(Actor.InActor);
        Data.RunningTime = 0.f;
        Data.LeftRunningTime = 0.f;
        Data.RightRunningTime = 0.f;

        // ----- STEP 2.1: Capture snapshot -----
        if (Cast<UPrimitiveComponent>(Actor.InActor->GetRootComponent()) && !Actor.InActor->IsA<ACharacter>())
        {
            auto* MeshRoot = Cast<UPrimitiveComponent>(Actor.InActor->GetRootComponent());
            Snapshot = {
                Actor.InActor->GetActorLocation(),
                Actor.InActor->GetActorRotation(),
                MeshRoot->GetPhysicsLinearVelocity(),
                MeshRoot->GetPhysicsAngularVelocityInRadians(),
                DeltaTime
            };
        }
        else
        {
           auto* Character = Cast<ACharacter>(Actor.InActor.Get());
           	
               FPoseSnapshot PoseSnapshot{};
           	
               Character->GetMesh()->SnapshotPose(PoseSnapshot);
   
               Snapshot = {
                   Actor.InActor->GetActorLocation(),
                   Actor.InActor->GetActorRotation(),
                   Character->GetCapsuleComponent()->GetPhysicsLinearVelocity(),
                   Character->GetCapsuleComponent()->GetPhysicsAngularVelocityInRadians(),
                   DeltaTime
               };
               Snapshot.PoseSnapshot = PoseSnapshot; 
        }

        // ----- STEP 2.2: Store snapshot -----
        if (Data.RecordedTime < RecordedTimeSeconds)
        {
            Data.StoredFrames.AddTail(Snapshot);
            Data.RecordedTime += DeltaTime;
            Data.bOutOfData = false;
        }
        else
        {
            while (Data.RecordedTime >= RecordedTimeSeconds)
            {
                auto& Frames = Data.StoredFrames;
                if (!Frames.GetHead()) break;

                float HeadDT = Frames.GetHead()->GetValue().DeltaTime;
                Frames.RemoveNode(Frames.GetHead());
                Data.RecordedTime -= HeadDT;
            }

            Data.StoredFrames.AddTail(Snapshot);
            Data.RecordedTime += DeltaTime;
            Data.bOutOfData = false;
        }
    }
}

void URewindSubsystem::HandleReversePlayback(float DeltaTime)
{
	
	TRACE_CPUPROFILER_EVENT_SCOPE(URewindSubsystem_Rewind);

	/*if (bIsExecutingThreadTask)
	{
		return;
	}*/
	
	auto RewindSpeed{GetDefault<URewindDeveloperSettings>()->GetRewindSpeed()};

	int32 ValidActorCount{};
	float AvgFramesRemaining{};

	
	 for (auto& Actor : ReverseActors)
	 {
	 	if (!Actor.IsValid()) continue;

	 	auto* Data = ActorsData.Find(Actor.InActor);

	 	const bool IsCharacter = Actor.InActor->IsA<ACharacter>();
	 	
	 	if (!Data || Data->StoredFrames.IsEmpty() || Data->bOutOfData) continue;

	 	int32 TotalFrames = 0;

	 	if (!Data->StoredFrames.IsEmpty() && !Data->bOutOfData)
	 	{
	 		TotalFrames += Data->StoredFrames.Num();
	 		++ValidActorCount;
	 	}

	 	AvgFramesRemaining = ValidActorCount > 0 ? static_cast<float>(TotalFrames) / ValidActorCount : 0.f;

	 	// ----- STEP 3.1: Locate snapshot pair -----
	 	auto IsCurveSet=RewindConfig.IsCurveSet();
	 	Data->RunningTime += DeltaTime * (IsCurveSet?RewindConfig.RewindCurve->GetFloatValue(Data->RunningTime):RewindSpeed);

	 	auto Right = Data->StoredFrames.GetTail();
	 	auto Left = Right->GetPrevNode();
	 	Data->LeftRunningTime = Data->RightRunningTime + Right->GetValue().DeltaTime;

	 	while (Data->RunningTime > Data->LeftRunningTime)
	 	{

	 		Data->RightRunningTime += Right->GetValue().DeltaTime;
	 		Right = Left;
	 		Data->LeftRunningTime += Left->GetValue().DeltaTime;
	 		Left = Left->GetPrevNode();

	 		auto Tail = Data->StoredFrames.GetTail();
	 		Data->RecordedTime -= Tail->GetValue().DeltaTime;
	 		Data->StoredFrames.RemoveNode(Tail);

	 		if (Left == Data->StoredFrames.GetHead())
	 		{
	 			Data->bOutOfData = true;
	 		}
	 	}

	 	// ----- STEP 3.2: Interpolate and apply snapshot -----
	 	if (Data->RunningTime <= Data->LeftRunningTime && Data->RunningTime >= Data->RightRunningTime)
	 	{
	 		TRACE_CPUPROFILER_EVENT_SCOPE(URewindSubsystem_CalculateInterp);

        	
        	
	 		const float DeltaTimeAdjusted = Data->RunningTime - Data->RightRunningTime;
	 		const float Interval = Data->LeftRunningTime - Data->RightRunningTime;
	 		const float Fraction = DeltaTimeAdjusted / Interval;

	 		//bIsExecutingThreadTask=true;

	 		FRewindedActorFrameSnapshot RewindedActorFrameSnapshot{};

	 		RewindedActorFrameSnapshot.Location = FMath::Lerp(
			   Right->GetValue().Location, Left->GetValue().Location, Fraction);

	 		RewindedActorFrameSnapshot.Rotation = FMath::Lerp(
				 Right->GetValue().Rotation, Left->GetValue().Rotation, Fraction);

	 		RewindedActorFrameSnapshot.LinearVelocity = FMath::Lerp(
				 Right->GetValue().LinearVelocity, Left->GetValue().LinearVelocity, Fraction);

	 		RewindedActorFrameSnapshot.AngularVelocity = FMath::Lerp(
				 Right->GetValue().AngularVelocity, Left->GetValue().AngularVelocity, Fraction);

	 		RewindedActorFrameSnapshot.PoseSnapshot = InterpPoseSnapshotTo(Right->GetValue().PoseSnapshot,Left->GetValue().PoseSnapshot,Fraction, 1.f);
	 		
	 		if (IsCharacter)
	 		{
	 			Actor.InRewindComponent->TargetPose = RewindedActorFrameSnapshot.PoseSnapshot;
	 		}

	 		SetSnapshotVariables(
				 Actor.InActor.Get(), RewindedActorFrameSnapshot.Location,  RewindedActorFrameSnapshot.Rotation, RewindedActorFrameSnapshot.LinearVelocity, RewindedActorFrameSnapshot.AngularVelocity);


	 		
	 		/*AsyncTask(ENamedThreads::Type::AnyHiPriThreadNormalTask, [this, Right, Left, Fraction, Actor,IsCharacter]()
			 {

				 AsyncTask(ENamedThreads::Type::GameThread, [this,IsCharacter, Right, Left, Fraction, Actor, InterpSnapshot, InterpLocation, InterpRotation, InterpLinearVelocity, InterpAngularVelocity]()
				 {
				 	bIsExecutingThreadTask=false;
					
				 });
		 });*/
	 		
		 }
	 }
	//if the avrg frames remaining of all actors pass the MinAvgThreshold, end the rewind
	if (AvgFramesRemaining < RewindConfig.MinAvgThreshold)
	{
		EndReverse();
	}
}

void URewindSubsystem::CalculateSnapshot(FRewindedActorFrameSnapshot& SnapshotOut, bool bIsCharacter)
{
	
	
	
}



void URewindSubsystem::RemovePendingKillActorsOrRequested()
{
	
	for (auto& Data : ReverseActors)
	{
		if (!Data.InActor.IsValid())
		{
			PendingRemoveActors.Add(Data.InActor);
		}
	}
	
	for (TWeakObjectPtr<AActor>& Actor : PendingRemoveActors)
	{
		if (!Actor.IsValid()) continue;
		
		auto Temp=FRewindActor(Actor.Get(), nullptr);// Assuming null component is fine
		
		ReverseActors.Remove(Temp);
		
		ActorsData.Remove(Actor.Get());
	}

	
	if (PendingRemoveActors.Num()>0)
	{
		UE_LOGFMT(LogRewind,Warning,"Request Removed or Pending Kill Removed Count: {Num}",PendingRemoveActors.Num());
	}
	
	PendingRemoveActors.Reset();
}


void URewindSubsystem::StartReverse()
{
	bRewindingTime = true;

	//OnStartReverse.Broadcast();
	
	TRACE_BOOKMARK(TEXT("URewindSubsystem::StartReverse"))
	
	for (auto& Actor :ReverseActors)
	{
		if (!Actor.IsValid()) continue;

		Actor.InRewindComponent->bReversingTime=true;
		Actor.InRewindComponent->OnStartReverseTime.Broadcast();
	}
	
}

void URewindSubsystem::EndReverse()
{
	bRewindingTime = false;
	
	TRACE_BOOKMARK(TEXT("URewindSubsystem::EndReverse"))
	
	//OnEndReverse.Broadcast();
    
	for (auto& Actor :ReverseActors)
	{
		if (!Actor.IsValid()) continue;

		Actor.InRewindComponent->bReversingTime=false;
		Actor.InRewindComponent->OnEndReverseTime.Broadcast();
	}
}
