// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Input/UxtPointerTypes.h"
#include "UxtNearPointerComponent.generated.h"

struct FUxtGrabPointerFocus;
struct FUxtTouchPointerFocus;

/**
 * Adds touch and grab interactions to an actor.
 * It keeps track of all overlapping touch targets and raises focus events on the closest one.
 * Targets use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtNearPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UUxtNearPointerComponent();
	virtual ~UUxtNearPointerComponent();

	// 
	// UActorComponent interface

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void SetActive(bool bNewActive, bool bReset = false) override;

	UFUNCTION(BlueprintGetter)
	EControllerHand GetHand() const;
	UFUNCTION(BlueprintSetter)
	void SetHand(EControllerHand NewHand);

	UFUNCTION(BlueprintGetter)
	ECollisionChannel GetTraceChannel() const;
	UFUNCTION(BlueprintSetter)
	void SetTraceChannel(ECollisionChannel NewTraceChannel);

	UFUNCTION(BlueprintGetter)
	float GetProximityRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetProximityRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetTouchRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetTouchRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetGrabRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetGrabRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetTouchDepth() const;
	UFUNCTION(BlueprintSetter)
	void SetTouchDepth(float depth);

	/** Returns currently focused grab target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const;

	/** Returns currently focused touch target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedTouchTarget(FVector& OutClosestPointOnTarget) const;

	/**
	 * Set a focused grab target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/**
	 * Set a focused touch target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedTouchTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/** Returns whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintGetter)
	bool GetFocusLocked() const;

	/** Sets whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintSetter)
	void SetFocusLocked(bool Value);

	UFUNCTION(BlueprintGetter)
	bool IsGrabbing() const;
    
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	bool GetIsTouching() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetGrabPointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetTouchPointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	float GetTouchPointerRadius() const;

protected:

	/** Focus of the grab pointer */
	FUxtGrabPointerFocus* GrabFocus;

	/** Focus of the touch pointer */
	FUxtTouchPointerFocus* TouchFocus;
    
    /** Weak reference to the currently touched target. */
	TWeakObjectPtr<UActorComponent> TouchTargetWeak;

	/** Weak reference to the currently touched target primitive. */
	TWeakObjectPtr<UPrimitiveComponent> TouchPrimitiveWeak;

private:

	/** The hand that this component represents.
	 *  Determines the position of touch and grab pointers.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetHand", BlueprintSetter = "SetHand", Category = "Hand Pointer")
	EControllerHand Hand = EControllerHand::Right;

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetTraceChannel", BlueprintSetter = "SetTraceChannel", Category = "Hand Pointer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintSetter = "SetProximityRadius", BlueprintGetter = "GetProximityRadius", Category = "Hand Pointer")
	float ProximityRadius = 11.0f;
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetTouchRadius", BlueprintGetter = "GetTouchRadius", Category = "Hand Pointer")
	float TouchRadius = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetGrabRadius", BlueprintGetter = "GetGrabRadius", Category = "Hand Pointer")
	float GrabRadius = 3.5f;

	/**
	 * The depth beyond the front face at which a front face touchable no longer recieves touch events.
	 * While touching a front face touchable, if the near pointer moves beyond this depth, the touchable
	 * will receive a touch end event.
	 */
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetTouchDepth", BlueprintGetter = "GetTouchDepth", Category = "Hand Pointer")
	float TouchDepth = 20.0f;

	/**
	 * Whether the pointer is locked on its current focused target.
	 * When locked, pointers won't change their focused target even if they stop overlapping it.
	 */
	UPROPERTY(BlueprintGetter = "GetFocusLocked", BlueprintSetter = "SetFocusLocked", Category = "Hand Pointer")
	bool bFocusLocked = false;

	bool bIsTouching = false;

	FVector PreviousTouchPointerLocation;

	bool bWasBehindFrontFace = false;
};
