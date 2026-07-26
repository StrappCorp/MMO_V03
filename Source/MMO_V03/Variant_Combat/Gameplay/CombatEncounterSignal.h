// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatActivatable.h"
#include "CombatEncounterSignal.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Minimal activatable beacon used to signal encounter state changes in the combat slice.
 */
UCLASS()
class ACombatEncounterSignal : public AActor, public ICombatActivatable
{
	GENERATED_BODY()

public:
	ACombatEncounterSignal();

	virtual void OnConstruction(const FTransform& Transform) override;

	void SetInactiveLabel(const FText& InText);
	void SetActiveLabel(const FText& InText);

	// ~Begin ICombatActivatable interface
	virtual void ToggleInteraction(AActor* ActivationInstigator) override;
	virtual void ActivateInteraction(AActor* ActivationInstigator) override;
	virtual void DeactivateInteraction(AActor* ActivationInstigator) override;
	// ~End ICombatActivatable interface

protected:
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPointLightComponent> SignalLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(EditAnywhere, Category="Signal")
	FText InactiveLabel;

	UPROPERTY(EditAnywhere, Category="Signal")
	FText ActiveLabel;

	UPROPERTY(EditAnywhere, Category="Signal")
	FLinearColor InactiveColor;

	UPROPERTY(EditAnywhere, Category="Signal")
	FLinearColor ActiveColor;

	UPROPERTY(EditAnywhere, Category="Signal")
	bool bIsActive = false;
};
