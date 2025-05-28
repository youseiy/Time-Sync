// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RewindDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game,DefaultConfig, meta=(DisplayName="Rewind Settings"))
class REWIND_API URewindDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()


public:
	float GetRewindSpeed() const;
	float GetRecordedTimeSeconds() const;
	TSoftObjectPtr<UCurveFloat> GetRewindCurveFloat() const;
	
private:
	UPROPERTY(EditAnywhere,Config)
	float RecordTime{15.f};
	UPROPERTY(EditAnywhere,Config)
	float RewindSpeed{1.f};
	UPROPERTY(EditAnywhere,Config)
	TSoftObjectPtr<UCurveFloat> RewindCurve;
};
