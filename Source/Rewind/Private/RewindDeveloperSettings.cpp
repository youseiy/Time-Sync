// Fill out your copyright notice in the Description page of Project Settings.


#include "RewindDeveloperSettings.h"

float URewindDeveloperSettings::GetRewindSpeed() const
{
	return RewindSpeed;
}

float URewindDeveloperSettings::GetRecordedTimeSeconds() const
{
	return RecordTime;
}

TSoftObjectPtr<UCurveFloat> URewindDeveloperSettings::GetRewindCurveFloat() const
{
	return RewindCurve;
}
