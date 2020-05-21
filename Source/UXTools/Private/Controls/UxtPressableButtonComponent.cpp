// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableButtonComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "UXTools.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include <GameFramework/Actor.h>
#include <DrawDebugHelpers.h>
#include <Components/BoxComponent.h>
#include <Components/ShapeComponent.h>
#include <Components/StaticMeshComponent.h>

// Sets default values for this component's properties
UUxtPressableButtonComponent::UUxtPressableButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

float UUxtPressableButtonComponent::GetFrontFaceCollisionMargin() const
{
	return FrontFaceCollisionMargin;
}

void UUxtPressableButtonComponent::SetFrontFaceCollisionMargin(float Distance)
{
	Distance = FMath::Max(0.0f, Distance);

	FrontFaceCollisionMargin = Distance;
	if (BoxComponent)
	{
		if (UStaticMeshComponent* Touchable = Cast<UStaticMeshComponent>(GetVisuals()))
		{
			ConfigureBoxComponent(Touchable);
		}
	}
}

USceneComponent* UUxtPressableButtonComponent::GetVisuals() const 
{
	return Cast<USceneComponent>(VisualsReference.GetComponent(GetOwner()));
}

void UUxtPressableButtonComponent::SetVisuals(USceneComponent* Visuals)
{
	VisualsReference.OverrideComponent = Visuals;

	if (Visuals)
	{
		ConfigureBoxComponent(Visuals);
	}
}

void UUxtPressableButtonComponent::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;
	if (BoxComponent)
	{
		BoxComponent->SetCollisionProfileName(CollisionProfile);
	}
}

void UUxtPressableButtonComponent::SetEnabled(bool Enabled)
{
	if (Enabled && State == EUxtButtonState::Disabled)
	{
		State = EUxtButtonState::Default;
		OnButtonEnabled.Broadcast(this);
	}
	else if (!Enabled && State != EUxtButtonState::Disabled)
	{
		// Free any locked pointers
		if (FarPointerWeak.Get())
		{
			FarPointerWeak->SetFocusLocked(false);
			FarPointerWeak = nullptr;
		}
		PokePointers.Empty();
		CurrentPushDistance = 0;

		State = EUxtButtonState::Disabled;
		OnButtonDisabled.Broadcast(this);
	}
}

bool UUxtPressableButtonComponent::IsEnabled() const
{
	return State != EUxtButtonState::Disabled;
}

bool UUxtPressableButtonComponent::IsPressed() const
{
	return State == EUxtButtonState::Pressed;
}

bool UUxtPressableButtonComponent::IsFocused() const
{
	return NumPointersFocusing > 0;
}

float UUxtPressableButtonComponent::GetScaleAdjustedMaxPushDistance() const
{
	return MaxPushDistance * GetComponentTransform().GetScale3D().X;
}

float UUxtPressableButtonComponent::GetMaxPushDistance() const
{
	return MaxPushDistance;
}

void UUxtPressableButtonComponent::SetMaxPushDistance(float Distance)
{
	MaxPushDistance = Distance;
}

// Called when the game starts
void UUxtPressableButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent = NewObject<UBoxComponent>(this);

	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	if (USceneComponent* Visuals = GetVisuals())
	{
		ConfigureBoxComponent(Visuals);
	}
}

// Called every frame
void UUxtPressableButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update poke if we're not currently pressed via a far pointer
	if (!FarPointerWeak.IsValid())
	{
		// Update button logic with all known pointers
		UUxtNearPointerComponent* NewPokingPointer = nullptr;
		float TargetDistance = 0;

		for (UUxtNearPointerComponent* Pointer : PokePointers)
		{
			float PushDistance = CalculatePushDistance(Pointer);
			if (PushDistance > TargetDistance)
			{
				NewPokingPointer = Pointer;
				TargetDistance = PushDistance;
			}
		}

		check(TargetDistance >= 0 && TargetDistance <= MaxPushDistance);

		const float PreviousPushDistance = CurrentPushDistance;

		// Update push distance and raise events
		if (TargetDistance > CurrentPushDistance)
		{
			CurrentPushDistance = TargetDistance;
			float PressedDistance = GetPressedDistance();

			if (State != EUxtButtonState::Pressed && CurrentPushDistance >= PressedDistance && PreviousPushDistance < PressedDistance)
			{
				State = EUxtButtonState::Pressed;
				OnButtonPressed.Broadcast(this, NewPokingPointer);
			}
		}
		else
		{
			CurrentPushDistance = FMath::Max(TargetDistance, CurrentPushDistance - DeltaTime * RecoverySpeed);
			float ReleasedDistance = GetReleasedDistance();

			// Raise button released if we're pressed and crossed the released distance
			if (State == EUxtButtonState::Pressed && (CurrentPushDistance <= ReleasedDistance && PreviousPushDistance > ReleasedDistance))
			{
				State = EUxtButtonState::Default;
				OnButtonReleased.Broadcast(this, NewPokingPointer);
			}
		}
	}

	// Update visuals behaviors
	if (USceneComponent* Visuals = GetVisuals())
	{
		switch (PushBehavior)
		{
		default:
		case EUxtPushBehavior::Translate:
		{
			const FVector VisualsOffset = GetComponentTransform().TransformVector(VisualsOffsetLocal);
			FVector NewVisualsLocation = VisualsOffset + GetCurrentButtonLocation();
			Visuals->SetWorldLocation(NewVisualsLocation);
		}
		break;
		case EUxtPushBehavior::Compress:
		{
			float CompressionScale = (MaxPushDistance != 0.0f) ? 1.0f - (CurrentPushDistance / MaxPushDistance) : 1.0f;
			CompressionScale = FMath::Clamp(CompressionScale, PressedFraction, 1.0f);
			Visuals->SetRelativeScale3D(FVector(VisualsScaleLocal.X * CompressionScale, VisualsScaleLocal.Y, VisualsScaleLocal.Z));
		}
		break;
		}
	}

#if 0
	// Debug display
	{
		// Button face
		{
			FVector Position = GetCurrentButtonLocation();
			FPlane Plane(Position, -GetComponentTransform().GetUnitAxis(EAxis::X));
			FVector2D HalfExtents = GetButtonExtents();
			DrawDebugSolidPlane(GetWorld(), Plane, Position, HalfExtents, FColor::Blue);
		}

		// Pointers
		for (const auto& Pointer : GetPokePointers())
		{
			auto Position = Pointer.Key->GetPokePointerTransform().GetLocation();

			// Shift it up a bit so it is not hidden by the pointer visuals.
			Position.Z += 2;

			DrawDebugPoint(GetWorld(), Position, 10, FColor::Yellow);
		}
	}
#endif
}

#if WITH_EDITOR
bool UUxtPressableButtonComponent::CanEditChange(const FProperty* Property) const
{
	bool IsEditable = Super::CanEditChange(Property);

	if (IsEditable && Property != nullptr)
	{
		// When a button's push behavior is compressible the max push distance is auto-calculated and should not be 
		// edited by the user.
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUxtPressableButtonComponent, MaxPushDistance))
		{
			IsEditable = PushBehavior != EUxtPushBehavior::Compress;
		}
	}

	return IsEditable;
}
#endif

void UUxtPressableButtonComponent::OnEnterFocus(UObject* Pointer)
{
	const bool bWasFocused = ++NumPointersFocusing > 1;
	OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
}

void UUxtPressableButtonComponent::OnExitFocus(UObject* Pointer)
{
	--NumPointersFocusing;
	const bool bIsFocused = IsFocused();

	if (!bIsFocused)
	{
		if (State == EUxtButtonState::Pressed)
		{
			State = EUxtButtonState::Default;
			OnButtonReleased.Broadcast(this, Pointer);
		}
	}

	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

bool UUxtPressableButtonComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return Primitive == BoxComponent;
}

void UUxtPressableButtonComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtPressableButtonComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (State != EUxtButtonState::Disabled)
	{
		// Lock the poking pointer so we remain the focused target as it moves.
		Pointer->SetFocusLocked(true);

		PokePointers.Add(Pointer);
		OnBeginPoke.Broadcast(this, Pointer);
	}
}

void UUxtPressableButtonComponent::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (State != EUxtButtonState::Disabled)
	{
		OnUpdatePoke.Broadcast(this, Pointer);
	}
}

void UUxtPressableButtonComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (State == EUxtButtonState::Pressed && NumPointersFocusing == 0)
	{
		State = EUxtButtonState::Default;
		OnButtonReleased.Broadcast(this, Pointer);
	}

	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	PokePointers.Remove(Pointer);

	if (State != EUxtButtonState::Disabled)
	{
		OnEndPoke.Broadcast(this, Pointer);
	}
}

EUxtPokeBehaviour UUxtPressableButtonComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

float UUxtPressableButtonComponent::CalculatePushDistance(const UUxtNearPointerComponent* pointer) const
{
	FVector PointerPos = pointer->GetPokePointerTransform().GetLocation();
	PointerPos.X += pointer->GetPokePointerRadius();
	const FVector PointerLocal = GetComponentTransform().InverseTransformPosition(PointerPos);
	const float EndDistance = RestPositionLocal.X - PointerLocal.X;

	return EndDistance > 0 ? FMath::Min(EndDistance, MaxPushDistance) : 0;
}

FVector UUxtPressableButtonComponent::GetCurrentButtonLocation() const
{
	return GetRestPosition() - (GetComponentTransform().GetScaledAxis(EAxis::X) * CurrentPushDistance);
}


FVector UUxtPressableButtonComponent::GetRestPosition() const
{
	return GetComponentTransform().TransformPosition(RestPositionLocal);
}

bool UUxtPressableButtonComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return Primitive == BoxComponent;
}

void UUxtPressableButtonComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}


float UUxtPressableButtonComponent::GetPressedDistance() const
{
	return MaxPushDistance * PressedFraction;
}


float UUxtPressableButtonComponent::GetReleasedDistance() const
{
	return MaxPushDistance * ReleasedFraction;
}

void UUxtPressableButtonComponent::ConfigureBoxComponent(USceneComponent* Parent)
{
	if (!BoxComponent)
	{
		UE_LOG(UXTools, Error, TEXT("Attempting to configure the box component for '%s' before it is initialised, the button will not work properly."), *GetOwner()->GetName());
		return;
	}

	// Disable collision on all primitive components.
	TArray<USceneComponent*> SceneComponents;
	Parent->GetChildrenComponents(true, SceneComponents);
	SceneComponents.Add(Parent);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(SceneComponent))
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Get bounds local to button, not visuals
	FTransform LocalToTarget = Parent->GetComponentTransform() * GetComponentTransform().Inverse();
	FBoxSphereBounds LocalBounds = UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(Parent, LocalToTarget);
	FBox LocalBoxBounds = LocalBounds.GetBox();

	// Expand box to include the front face margin
	LocalBoxBounds = LocalBoxBounds.ExpandBy(FVector::ZeroVector, FVector::ForwardVector * FrontFaceCollisionMargin);

	FTransform BoxTransform = FTransform(LocalBoxBounds.GetCenter()) * GetComponentTransform();
	BoxComponent->SetWorldTransform(BoxTransform);
	BoxComponent->SetBoxExtent(LocalBoxBounds.GetExtent());
	BoxComponent->SetCollisionProfileName(CollisionProfile);
	BoxComponent->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);

	FVector RestPosition = BoxTransform.GetLocation() + BoxTransform.GetUnitAxis(EAxis::X) * BoxComponent->GetScaledBoxExtent().X;
	RestPositionLocal = GetComponentTransform().InverseTransformPosition(RestPosition);

	const FVector VisualsOffset = Parent->GetComponentLocation() - GetRestPosition();
	VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	VisualsScaleLocal = Parent->GetRelativeScale3D();

	// When the button is compressible, the max push distance is the 'x' bounds.
	if (PushBehavior == EUxtPushBehavior::Compress)
	{
		SetMaxPushDistance(BoxComponent->GetScaledBoxExtent().X * 2.0f);
	}
}

void UUxtPressableButtonComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!FarPointerWeak.IsValid() && State != EUxtButtonState::Disabled)
	{
		CurrentPushDistance = GetPressedDistance();
		FarPointerWeak = Pointer;
		Pointer->SetFocusLocked(true);
		OnButtonPressed.Broadcast(this, Pointer);
	}
}

void UUxtPressableButtonComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get();
	if (Pointer == FarPointer)
	{
		CurrentPushDistance = 0;
		FarPointerWeak = nullptr;
		Pointer->SetFocusLocked(false);

		if (State != EUxtButtonState::Disabled)
		{
			OnButtonReleased.Broadcast(this, Pointer);
		}
	}
}