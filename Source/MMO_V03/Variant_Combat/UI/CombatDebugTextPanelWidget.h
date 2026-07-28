#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatDebugTextPanelWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

UCLASS()
class UCombatDebugTextPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPanelPosition(const FVector2D& InPanelPosition);
	void SetPanelText(const FString& InHeaderText, const FString& InBodyText, const FString& InFooterText);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildWidgetTree();
	void RefreshDisplay();
	void ApplyPanelPosition();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FooterText;

	FVector2D PanelPosition = FVector2D(24.0f, 24.0f);
	FString CurrentHeaderText;
	FString CurrentBodyText;
	FString CurrentFooterText;
};
