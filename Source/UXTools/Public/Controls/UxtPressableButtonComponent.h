// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Interactions/UxtInteractableComponent.h"
#include "UxtPressableButtonComponent.generated.h"

namespace Microsoft
{
    namespace MixedReality
    {
        namespace UX
        {
            class PressableButton;
        }
    }
}

class UUxtPressableButtonComponent;
class UUxtFarPointerComponent;
class UShapeComponent;
struct FButtonHandler;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonPressedDelegate, UUxtPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonReleasedDelegate, UUxtPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtPressableButtonComponent : public UUxtInteractableComponent
{
	GENERATED_BODY()

public:	

	UUxtPressableButtonComponent();

	/** Get scene component used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	USceneComponent* GetVisuals() const;

	/** Set scene component to be used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetVisuals(USceneComponent* Visuals);

	/** Get the current pressed state of the button */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	bool IsPressed() const;

protected:

    //
    // UActorComponent interface

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//
	// IUxtFarTarget interface

	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;

public:

	/** 
	 * The extents (i.e. half the dimensions) of the button movement box.
	 * The X extent is the maximum travel distance for the button, Y and Z are the button width and height respectively.
	 */
	UPROPERTY(EditAnywhere, Category = "Pressable Button")
	FVector Extents;

	/** Fraction of the maximum travel distance at which the button will raise the pressed event. */
    UPROPERTY(EditAnywhere, Category = "Pressable Button")
    float PressedFraction;

	/** Fraction of the maximum travel distance at which a pressed button will raise the released event. */
    UPROPERTY(EditAnywhere, Category = "Pressable Button")
    float ReleasedFraction;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonPressedDelegate OnButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonReleasedDelegate OnButtonReleased;

private:

	FVector GetVisualsRestPosition() const;

	/** Visual representation of the button face. This component's transform will be updated as the button is pressed/released. */
	UPROPERTY(EditAnywhere, DisplayName = "Visuals", meta = (UseComponentPicker, AllowedClasses = "SceneComponent"), Category = "Pressable Button")
	FComponentReference VisualsReference;

	/** Far pointer currently pressing the button if any */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

    Microsoft::MixedReality::UX::PressableButton* Button = nullptr;
    FButtonHandler* ButtonHandler = nullptr;

	/** Visuals offset in this component's space */
	FVector VisualsOffsetLocal;
};
