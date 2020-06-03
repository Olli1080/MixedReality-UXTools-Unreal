// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtFarTarget.h"
#include "UxtPinchSliderComponent.generated.h"

UENUM(BlueprintType)
enum class EUxtSliderState : uint8
{
	/** Slider is not interacting */
	Default,
	/** Slider is in focus state */
	Focus,
	/** Slider is in focus state */
	Grab,
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderOnValueUpdated, UUxtPinchSliderComponent*, Slider, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderOnInteractionStarted, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderOnInteractionEnded, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderOnFocusEntered, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderOnFocusExited, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderOnStateUpdated, EUxtSliderState, NewState);

/**
 * Component that implements a thumb slider UI and logic.
 */


UCLASS( ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtPinchSliderComponent : public USceneComponent, public IUxtGrabTarget, public IUxtFarTarget
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUxtPinchSliderComponent();

	/** Set collision profile for the slider thumb */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetCollisionProfile(FName Profile);

	/** Get the current state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	EUxtSliderState GetCurrentState() const { return CurrentState; }

	/** Get the current grabbed state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	bool IsGrabbed() const  {return CurrentState == EUxtSliderState::Grab;}

	/** Get the current focus state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	bool IsFocused() const { return CurrentState == EUxtSliderState::Focus; }

	/** Get Static Mesh Component used for the thumb visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UStaticMeshComponent* GetThumbVisuals() const;

	/** Get Static Mesh Component used for the track visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UStaticMeshComponent* GetTrackVisuals() const;

	/** Get Instanced Static Mesh Component used for the tick marks */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UInstancedStaticMeshComponent* GetTickMarkVisuals() const;

	/** Set Static Mesh Component used for the thumb visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetThumbVisuals(UStaticMeshComponent* Visuals);

	/** Set Static Mesh Component used for the track visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetTrackVisuals(UStaticMeshComponent* Visuals);

	/** Set Instanced Static Mesh Component used for the tick marks */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetTickMarkVisuals(UInstancedStaticMeshComponent* Visuals);

	//
	// Getters and setters

	UFUNCTION(BlueprintGetter)
	float GetSliderValue() const { return SliderValue; }
	UFUNCTION(BlueprintSetter)
	void  SetSliderValue(float NewValue);
		
	UFUNCTION(BlueprintGetter)
	int GetNumTickMarks() const { return NumTickMarks; }
	UFUNCTION(BlueprintSetter)
	void  SetNumTickMarks(int NumTicks);

	UFUNCTION(BlueprintGetter)
	float GetSliderStartDistance() const { return SliderStartDistance; }
	UFUNCTION(BlueprintSetter)
	void  SetSliderStartDistance(float NewStart);

	UFUNCTION(BlueprintGetter)
	float GetSliderEndDistance() const { return SliderEndDistance; }
	UFUNCTION(BlueprintSetter)
	void  SetSliderEndDistance(float NewEnd);

	UFUNCTION(BlueprintGetter)
	FVector GetTickMarkScale() const { return TickMarkScale; }
	UFUNCTION(BlueprintSetter)
	void  SetTickMarkScale(FVector NewScale);

#if WITH_EDITOR
	/** Editor update function - called by UE4*/
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/** Editor update function - called by UE4 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//
	// Events

	/** Event raised when slider value changes. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnValueUpdated OnValueUpdated;

	/** Event raised when slider starts interaction. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnInteractionStarted OnInteractionStarted;

	/** Event raised when slider ends interaction. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnInteractionEnded OnInteractionEnded;

	/** Event raised when slider enters focus */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnFocusEntered OnFocusEnter;

	/** Event raised when slider exits focus */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnFocusExited OnFocusExit;

	/** Event raised when slider changes state */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderOnStateUpdated OnStateUpdated;

protected:

	//
	// UActorComponent interface
	virtual void BeginPlay() override;

	// IUxtGrabTarget interface

	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface

	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override;

private:
	/** The current value of the slider in 0-1 range */
	UPROPERTY(EditAnywhere, DisplayName = "SliderValue", BlueprintGetter = "GetSliderValue", BlueprintSetter = "SetSliderValue", Category = "Pinch Slider")
	float SliderValue;

	/** Where the slider track starts, as distance from center along slider axis, in local space units. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderStartDistance", BlueprintGetter = "GetSliderStartDistance", BlueprintSetter = "SetSliderStartDistance", Category = "Pinch Slider")
	float SliderStartDistance;

	/** Where the slider track ends, as distance from center along slider axis, in local space units. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderEndDistance", BlueprintGetter = "GetSliderEndDistance", BlueprintSetter = "SetSliderEndDistance", Category = "Pinch Slider")
	float SliderEndDistance;

	/** Number of tick marks to add to the slider */
	UPROPERTY(EditAnywhere, DisplayName = "NumTickMarks", BlueprintGetter = "GetNumTickMarks", BlueprintSetter = "SetNumTickMarks", Category = "Pinch Slider")
	int NumTickMarks;

	/** Scale of the tick mark on the slider */
	UPROPERTY(EditAnywhere, DisplayName = "TickMarkScale", BlueprintGetter = "GetTickMarkScale", BlueprintSetter = "SetTickMarkScale", Category = "Pinch Slider")
	FVector TickMarkScale;

	/** Turns local space position to 0-1 slider scale */
	void UpdateSliderValueFromLocalPosition(float LocalValue);

	/**  Updates thumb position based off 0-1 slider scale */
	void UpdateThumbPositionFromSliderValue();

	/** Use the given mesh to adjust the box component extents. */
	void ConfigureBoxComponent(const UStaticMeshComponent* Mesh);

	/** Internal function to reinitialise component to new state */
	void UpdateSliderState();

	/** Visual representation of the slider thumb*/
	UPROPERTY(EditAnywhere, DisplayName = "ThumbVisuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference ThumbVisuals;

	/** Visual representation of the track*/
	UPROPERTY(EditAnywhere, DisplayName = "TrackVisuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference TrackVisuals;

	/** Visual representation of the tick marks*/
	UPROPERTY(EditAnywhere, DisplayName = "TickMarkVisuals", meta = (UseComponentPicker, AllowedClasses = "InstancedStaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference TickMarkVisuals;

	/** Collision profile used by the slider thumb */
	UPROPERTY(EditAnywhere, Category = "Pinch Slider")
	FName CollisionProfile;

	/** Far pointer currently grabbing the slider if any */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Collision volume used for determining grab events */
	UPROPERTY(Transient)
	class UBoxComponent* BoxComponent;

	/** World space start position for the hand in far grab */
	FVector GrabStartPositionWS;

	/** Local space start position for the thumb in far grab */
	float GrabThumbStartPositionLS;

	/** Current state of the slider */
	EUxtSliderState CurrentState;

	/** Number of pointers currently focusing the button. */
	int NumPointersFocusing = 0;

};
