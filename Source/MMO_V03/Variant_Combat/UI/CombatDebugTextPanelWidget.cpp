#include "UI/CombatDebugTextPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"

void UCombatDebugTextPanelWidget::SetPanelPosition(const FVector2D& InPanelPosition)
{
	PanelPosition = InPanelPosition;
	ApplyPanelPosition();
}

void UCombatDebugTextPanelWidget::SetPanelText(const FString& InHeaderText, const FString& InBodyText, const FString& InFooterText)
{
	CurrentHeaderText = InHeaderText;
	CurrentBodyText = InBodyText;
	CurrentFooterText = InFooterText;
	RefreshDisplay();
}

void UCombatDebugTextPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCombatDebugTextPanelWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetPadding(FMargin(16.0f, 14.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.94f));
	RootCanvas->AddChild(PanelBorder);
	ApplyPanelPosition();

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	PanelBorder->SetContent(ContentBox);

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
	ContentBox->AddChildToVerticalBox(HeaderText);
	if (UVerticalBoxSlot* HeaderSlot = Cast<UVerticalBoxSlot>(HeaderText->Slot))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
	HeaderText->SetAutoWrapText(true);

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyText"));
	ContentBox->AddChildToVerticalBox(BodyText);
	if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(BodyText->Slot))
	{
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
	BodyText->SetAutoWrapText(true);

	FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FooterText"));
	ContentBox->AddChildToVerticalBox(FooterText);
	FooterText->SetAutoWrapText(true);

	RefreshDisplay();
}

void UCombatDebugTextPanelWidget::RefreshDisplay()
{
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(CurrentHeaderText));
	}

	if (BodyText)
	{
		BodyText->SetText(FText::FromString(CurrentBodyText));
	}

	if (FooterText)
	{
		FooterText->SetText(FText::FromString(CurrentFooterText));
	}
}

void UCombatDebugTextPanelWidget::ApplyPanelPosition()
{
	if (UCanvasPanelSlot* PanelSlot = PanelBorder ? Cast<UCanvasPanelSlot>(PanelBorder->Slot) : nullptr)
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		PanelSlot->SetPosition(PanelPosition);
	}
}
