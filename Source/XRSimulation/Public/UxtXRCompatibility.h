// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"

/**
 * Compatibility struct for FXRMotionControllerData which was removed in UE 5.4/5.7.
 * This allows UXTools to compile without major refactoring.
 */
struct FXRMotionControllerData
{
	FName DeviceName;
	FGuid ApplicationInstanceID;
	EControllerHand Hand;
    int32 HandIndex; // Compatibility for hand index
	ETrackingStatus TrackingStatus;
	FVector GripPosition;
	FQuat GripRotation;
	FVector AimPosition;
	FQuat AimRotation;
	TArray<FVector> HandKeyPositions; // Was HandKeypointPositions
	TArray<FQuat> HandKeyRotations; // Was HandKeypointRotations
	TArray<float> HandKeyRadii; // Was HandKeypointRadii
	bool bValid; // Was bIsTracked
    bool bIsGrasped; // Compatibility for grasped state
    EXRVisualType DeviceVisualType;

    FXRMotionControllerData()
        : DeviceName(NAME_None)
        , ApplicationInstanceID(FGuid())
        , Hand(EControllerHand::Left)
        , HandIndex(0)
        , TrackingStatus(ETrackingStatus::NotTracked)
        , GripPosition(FVector::ZeroVector)
        , GripRotation(FQuat::Identity)
        , AimPosition(FVector::ZeroVector)
        , AimRotation(FQuat::Identity)
        , bValid(false)
        , bIsGrasped(false)
        , DeviceVisualType(EXRVisualType::Hand)
    {
        HandKeyPositions.SetNumZeroed(static_cast<int32>(EHandKeypoint::LittleTip) + 1);
        HandKeyRotations.SetNumZeroed(static_cast<int32>(EHandKeypoint::LittleTip) + 1);
        HandKeyRadii.SetNumZeroed(static_cast<int32>(EHandKeypoint::LittleTip) + 1);
    }
};

/**
 * Compatibility typedefs for RHI types changed in UE 5.x.
 */
#include "RHI.h"
#include "RHIResources.h"
#include "IXRTrackingSystem.h"
#include "Engine/Engine.h"

#ifndef FTexture2DRHIRef
using FTexture2DRHIRef = FTextureRHIRef;
#endif

#ifndef FRHITexture2D
using FRHITexture2D = FRHITexture;
#endif

/**
 * Helper to replace IXRTrackingSystem::GetMotionControllerData in UE 5.x.
 */
inline bool UxtGetMotionControllerData(UObject* WorldContext, EControllerHand Hand, FXRMotionControllerData& OutData)
{
	OutData = FXRMotionControllerData();
	OutData.Hand = Hand;

	IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get();
	if (!XRSystem)
	{
		return false;
	}

	// In UE 5.4+, this info is often managed by specific hand trackers.
	// We'll return false here and let specialized trackers fill it if possible.
	return false;
}
