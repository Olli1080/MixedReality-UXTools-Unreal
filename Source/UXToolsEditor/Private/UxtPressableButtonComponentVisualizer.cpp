// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtPressableButtonComponentVisualizer.h"
#include "UXToolsEditor.h"
#include <SceneManagement.h>
#include <Components/StaticMeshComponent.h>
#include "Utils/UxtMathUtilsFunctionLibrary.h"

namespace
{
	void DrawQuad(FPrimitiveDrawInterface* PDI, float Width, float Height, float PressedDistance, const FMatrix& Transform, const FLinearColor& Color, bool bDashed = false)
	{	
		FVector Vertices[] =
		{
			FVector(PressedDistance, Width, Height),
			FVector(PressedDistance, Width, -Height),
			FVector(PressedDistance, -Width, -Height),
			FVector(PressedDistance, -Width, Height)
		};

		for (int i = 0; i < 4; ++i)
		{
			FVector Start = Transform.TransformPosition(Vertices[i]);
			FVector End = Transform.TransformPosition(Vertices[(i + 1) % 4]);
			if (bDashed)
			{
				DrawDashedLine(PDI, Start, End, Color, 0.1, SDPG_Foreground);
			}
			else
			{
				PDI->DrawLine(Start, End, Color, SDPG_Foreground, 0.05f);
			}
		}
	}
}

void FUxtPressableButtonComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UUxtPressableButtonComponent* Button = Cast<const UUxtPressableButtonComponent>(Component))
	{
		if (USceneComponent* Visuals = Button->GetVisuals())
		{
			FBox Bounds = UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(Visuals).GetBox();

			FTransform ToFrontFace = FTransform(FVector(Bounds.Min.X, 0, 0));
			FMatrix FrontFaceMatrix = (ToFrontFace * Visuals->GetComponentTransform()).ToMatrixNoScale();

			FVector Extents = Bounds.GetExtent();
			Extents *= Visuals->GetComponentTransform().GetScale3D();

			// Rest position
			DrawQuad(PDI, Extents.Y, Extents.Z, 0, FrontFaceMatrix, FLinearColor::White);

			// Maximum push distance
			const float MaxPushDistance = Button->MaxPushDistance;
			FLinearColor DarkGray(.25f, .25f, .25f);
			DrawQuad(PDI, Extents.Y, Extents.Z, MaxPushDistance, FrontFaceMatrix, DarkGray);

			// Pressed distance
			float PressedDistance = MaxPushDistance * Button->PressedFraction;
			FLinearColor LightGray(.75f, .75f, .75f);
			DrawQuad(PDI, Extents.Y, Extents.Z, PressedDistance, FrontFaceMatrix, LightGray, true);
		}
	}
}
