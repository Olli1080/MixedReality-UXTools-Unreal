// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtManipulationFlags.h"
#include "UxtManipulatorComponentBase.h"

#include "UxtGenericManipulatorComponent.generated.h"
/**
 * Generic manipulator that supports both one- and two-handed interactions.
 *
 * One-handed interaction supports linear movement as well as rotation based on the orientation of the hand.
 * Rotation modes can be selected with different axes and pivot points.
 *
 * Two-handed interaction moves the object based on the center between hands.
 * The actor can be rotated based on the line between both hands and scaled based on the distance.
 */
UCLASS(ClassGroup = UXTools, HideCategories = (Grabbable, ManipulatorComponent), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtGenericManipulatorComponent : public UUxtManipulatorComponentBase
{
	GENERATED_BODY()

public:
	UUxtGenericManipulatorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintGetter)
	float GetSmoothingFactor() const;
	UFUNCTION(BlueprintSetter)
	void SetSmoothingFactor(float NewSmoothingFactor);

protected:
	void UpdateOneHandManipulation(float DeltaSeconds);
	void UpdateTwoHandManipulation(float DeltaSeconds);

	bool GetOneHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandScale(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;

	/** Compute orientation that invariant in camera space. */
	FQuat GetViewInvariantRotation() const;

	virtual void BeginPlay() override;

public:
	/** Enabled manipulation modes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator, meta = (Bitmask, BitmaskEnum = EUxtGenericManipulationMode))
	int32 ManipulationModes;

	/** Mode of rotation to use while using one hand only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator)
	EUxtOneHandRotationMode OneHandRotationMode;

	/** Enabled transformations in two-handed manipulation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator, meta = (Bitmask, BitmaskEnum = EUxtTransformMode))
	int32 TwoHandTransformModes;

	/** Controls the object's behavior when physics its being simulated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator, meta = (Bitmask, BitmaskEnum = EUxtReleaseBehavior))
	int32 ReleaseBehavior;

	/** The component to transform, will default to the root scene component if not specified */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = GenericManipulator)
	FComponentReference TargetComponent;

private:
	bool IsNearManipulation() const;

	UFUNCTION()
	void OnGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	UFUNCTION()
	void OnRelease(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	/**
	 * Motion smoothing factor to apply while manipulating the object.
	 *
	 * @see UUxtManipulatorComponentBase::SmoothTransform
	 *
	 * It is deactivated it by default, since pointers already perform basic smoothing.
	 */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetSmoothingFactor, BlueprintSetter = SetSmoothingFactor, Category = GenericManipulator,
		meta = (ClampMin = "0.0"))
	float SmoothingFactor = 0;

	/** Was the target simulating physics */
	bool bWasSimulatingPhysics = false;
};
