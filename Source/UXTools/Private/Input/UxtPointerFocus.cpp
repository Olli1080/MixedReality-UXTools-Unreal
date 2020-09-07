// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtPointerFocus.h"

#include "Components/PrimitiveComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Interactions/UxtPokeTarget.h"

bool FUxtPointerFocusSearchResult::IsValid() const
{
	return (Target != nullptr) && (Primitive != nullptr);
}

const FVector& FUxtPointerFocus::GetClosestTargetPoint() const
{
	return ClosestTargetPoint;
}

const FVector& FUxtPointerFocus::GetClosestTargetNormal() const
{
	return ClosestTargetNormal;
}

UObject* FUxtPointerFocus::GetFocusedTarget() const
{
	return FocusedTargetWeak.Get();
}

UPrimitiveComponent* FUxtPointerFocus::GetFocusedPrimitive() const
{
	return FocusedPrimitiveWeak.Get();
}

UObject* FUxtPointerFocus::GetFocusedTargetChecked() const
{
	if (UObject* Target = FocusedTargetWeak.Get())
	{
		if (ImplementsTargetInterface(Target))
		{
			return Target;
		}
	}
	return nullptr;
}

void FUxtPointerFocus::SelectClosestTarget(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const TArray<FOverlapResult>& Overlaps)
{
	FUxtPointerFocusSearchResult Result = FindClosestTarget(Overlaps, PointerTransform.GetLocation());
	SetFocus(Pointer, PointerTransform, Result);
}

void FUxtPointerFocus::UpdateClosestTarget(const FTransform& PointerTransform)
{
	if (UActorComponent* ClosesTarget = Cast<UActorComponent>(FocusedTargetWeak.Get()))
	{
		if (UPrimitiveComponent* Primitive = FocusedPrimitiveWeak.Get())
		{
			GetClosestPointOnTarget(ClosesTarget, Primitive, PointerTransform.GetLocation(), ClosestTargetPoint, ClosestTargetNormal);
		}
	}
}

void FUxtPointerFocus::SelectClosestPointOnTarget(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, UActorComponent* NewTarget)
{
	if (NewTarget)
	{
		bool IsValidTarget = ensureMsgf(
			ImplementsTargetInterface(NewTarget), TEXT("Target object must implement %s interface for finding the closest point"),
			*GetInterfaceClass()->GetName());

		if (IsValidTarget)
		{
			const FVector Point = PointerTransform.GetLocation();

			FUxtPointerFocusSearchResult Result = FindClosestPointOnComponent(NewTarget, PointerTransform.GetLocation());
			if (Result.IsValid())
			{
				SetFocus(Pointer, PointerTransform, Result);
			}
		}
	}
	else
	{
		ClearFocus(Pointer);
	}
}

void FUxtPointerFocus::ClearFocus(UUxtNearPointerComponent* Pointer)
{
	UObject* FocusedTarget = FocusedTargetWeak.Get();
	if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
	{
		RaiseExitFocusEvent(FocusedTarget, Pointer);
	}

	FocusedTargetWeak.Reset();
	FocusedPrimitiveWeak.Reset();
	ClosestTargetPoint = FVector::ZeroVector;
	ClosestTargetNormal = FVector::ForwardVector;
}

void FUxtPointerFocus::UpdateFocus(UUxtNearPointerComponent* Pointer) const
{
	if (UObject* FocusedTarget = GetFocusedTargetChecked())
	{
		RaiseUpdateFocusEvent(FocusedTarget, Pointer);
	}
}

void FUxtPointerFocus::SetFocus(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const FUxtPointerFocusSearchResult& FocusResult)
{
	UObject* FocusedTarget = FocusedTargetWeak.Get();
	UPrimitiveComponent* FocusedPrimitive = FocusedPrimitiveWeak.Get();

	// If focused target is unchanged, then update only the closest-point-on-target
	if (FocusResult.Target == FocusedTarget && FocusResult.Primitive == FocusedPrimitive)
	{
		ClosestTargetPoint = FocusResult.ClosestPointOnTarget;
		ClosestTargetNormal = FocusResult.Normal;
	}
	else
	{
		// Update focused target
		if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			RaiseExitFocusEvent(FocusedTarget, Pointer);
		}

		FocusedTarget = FocusResult.Target;
		FocusedTargetWeak = FocusResult.Target;
		FocusedPrimitiveWeak = FocusResult.Primitive;
		ClosestTargetPoint = FocusResult.ClosestPointOnTarget;
		ClosestTargetNormal = FocusResult.Normal;

		if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			RaiseEnterFocusEvent(FocusedTarget, Pointer);
		}
	}
}

/** Find a component of the actor that implements the given interface type. */
UActorComponent* FUxtPointerFocus::FindInterfaceComponent(AActor* Owner) const
{
	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (ImplementsTargetInterface(Component))
		{
			return Component;
		}
	}
	return nullptr;
}

FUxtPointerFocusSearchResult FUxtPointerFocus::FindClosestTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point) const
{
	float MinDistanceSqr = MAX_FLT;
	UActorComponent* ClosestTarget = nullptr;
	UPrimitiveComponent* ClosestPrimitive = nullptr;
	FVector ClosestPointOnTarget = FVector::ZeroVector;
	FVector ClosestNormal = FVector::ForwardVector;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Primitive = Overlap.GetComponent();

		for (UActorComponent* Component : Overlap.GetActor()->GetComponents())
		{
			if (ImplementsTargetInterface(Component))
			{
				FVector PointOnTarget;
				FVector Normal;

				if (GetClosestPointOnTarget(Component, Primitive, Point, PointOnTarget, Normal))
				{
					float DistanceSqr = (Point - PointOnTarget).SizeSquared();
					if (DistanceSqr < MinDistanceSqr)
					{
						MinDistanceSqr = DistanceSqr;
						ClosestTarget = Component;
						ClosestPrimitive = Primitive;
						ClosestPointOnTarget = PointOnTarget;
						ClosestNormal = Normal;
					}

					// We keep the first target component that takes ownership of the primitive.
					break;
				}
			}
		}
	}

	if (ClosestTarget != nullptr)
	{
		return {ClosestTarget, ClosestPrimitive, ClosestPointOnTarget, ClosestNormal, FMath::Sqrt(MinDistanceSqr)};
	}
	else
	{
		return {nullptr, nullptr, FVector::ZeroVector, FVector::ForwardVector, MAX_FLT};
	}
}

FUxtPointerFocusSearchResult FUxtPointerFocus::FindClosestPointOnComponent(UActorComponent* Target, const FVector& Point) const
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Target->GetOwner()->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	UPrimitiveComponent* ClosestPrimitive = nullptr;
	FVector ClosestPoint = FVector::ZeroVector;
	FVector ClosestNormal = FVector::ForwardVector;
	float MinDistanceSqr = -1.f;
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		FVector PointOnPrimitive;
		FVector Normal;
		GetClosestPointOnTarget(Target, Primitive, Point, PointOnPrimitive, Normal);

		float DistanceSqr = FVector::DistSquared(Point, PointOnPrimitive);
		if (!ClosestPrimitive || DistanceSqr < MinDistanceSqr)
		{
			ClosestPrimitive = Primitive;
			MinDistanceSqr = DistanceSqr;
			ClosestPoint = PointOnPrimitive;
			ClosestNormal = Normal;

			if (MinDistanceSqr <= KINDA_SMALL_NUMBER)
			{
				// Best result to be expected.
				break;
			}
		}
	}

	if (ClosestPrimitive != nullptr)
	{
		return {Target, ClosestPrimitive, ClosestPoint, ClosestNormal, FMath::Sqrt(MinDistanceSqr)};
	}
	else
	{
		return {nullptr, nullptr, FVector::ZeroVector, FVector::ForwardVector, MAX_FLT};
	}
}

void FUxtGrabPointerFocus::BeginGrab(UUxtNearPointerComponent* Pointer)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtGrabTarget::Execute_OnBeginGrab(Target, Pointer);
	}

	bIsGrabbing = true;
}

void FUxtGrabPointerFocus::UpdateGrab(UUxtNearPointerComponent* Pointer)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtGrabTarget::Execute_OnUpdateGrab(Target, Pointer);
	}
}

void FUxtGrabPointerFocus::EndGrab(UUxtNearPointerComponent* Pointer)
{
	bIsGrabbing = false;

	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtGrabTarget::Execute_OnEndGrab(Target, Pointer);
	}
}

bool FUxtGrabPointerFocus::IsGrabbing() const
{
	return bIsGrabbing;
}

UClass* FUxtGrabPointerFocus::GetInterfaceClass() const
{
	return UUxtGrabTarget::StaticClass();
}

bool FUxtGrabPointerFocus::ImplementsTargetInterface(UObject* Target) const
{
	return Target->Implements<UUxtGrabTarget>();
}

bool FUxtGrabPointerFocus::GetClosestPointOnTarget(
	const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint,
	FVector& OutNormal) const
{
	float NotUsed;
	if (FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed))
	{
		if (OutClosestPoint == Point)
		{
			OutNormal = Point - Primitive->GetComponentLocation();
		}
		else
		{
			OutNormal = Point - OutClosestPoint;
		}

		OutNormal.Normalize();
		return true;
	}
	return false;
}

void FUxtGrabPointerFocus::RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtGrabTarget::Execute_OnEnterGrabFocus(Target, Pointer);
}

void FUxtGrabPointerFocus::RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtGrabTarget::Execute_OnUpdateGrabFocus(Target, Pointer);
}

void FUxtGrabPointerFocus::RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtGrabTarget::Execute_OnExitGrabFocus(Target, Pointer);
}

void FUxtPokePointerFocus::BeginPoke(UUxtNearPointerComponent* Pointer)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtPokeTarget::Execute_OnBeginPoke(Target, Pointer);
	}

	bIsPoking = true;
}

void FUxtPokePointerFocus::UpdatePoke(UUxtNearPointerComponent* Pointer)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtPokeTarget::Execute_OnUpdatePoke(Target, Pointer);
	}
}

void FUxtPokePointerFocus::EndPoke(UUxtNearPointerComponent* Pointer)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtPokeTarget::Execute_OnEndPoke(Target, Pointer);
	}

	bIsPoking = false;
}

bool FUxtPokePointerFocus::IsPoking() const
{
	return bIsPoking;
}

UClass* FUxtPokePointerFocus::GetInterfaceClass() const
{
	return UUxtPokeTarget::StaticClass();
}

bool FUxtPokePointerFocus::ImplementsTargetInterface(UObject* Target) const
{
	return Target->Implements<UUxtPokeTarget>();
}

bool FUxtPokePointerFocus::GetClosestPointOnTarget(
	const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint,
	FVector& OutNormal) const
{
	return IUxtPokeTarget::Execute_IsPokeFocusable(Target, Primitive) &&
		   IUxtPokeTarget::Execute_GetClosestPoint(Target, Primitive, Point, OutClosestPoint, OutNormal);
}

void FUxtPokePointerFocus::RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtPokeTarget::Execute_OnEnterPokeFocus(Target, Pointer);
}

void FUxtPokePointerFocus::RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtPokeTarget::Execute_OnUpdatePokeFocus(Target, Pointer);
}

void FUxtPokePointerFocus::RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtPokeTarget::Execute_OnExitPokeFocus(Target, Pointer);
}