// Copyright 2018 Phosphor Studios. All Rights Reserved.

#include "TutorialManager.h"
#include "TimerManager.h"
#include "PlayerController.h"
#include "Geometry.h"
#include "SlateBlueprintLibrary.h"
#include "WidgetLayoutLibrary.h"
#include "PhoButton.h"
#include "CanvasPanelSlot.h"
#include "CloseWidget.h"
#include "TutorialItem.h"
#include "TutorialTemplate.h"
#include "TutorialDialogueWidget.h"
#include "HUDBase.h"
#include "Widget.h"
#include "PlayerProfileTags.h"
#include "PlayerProfile.h"
#include "TownManager.h"
#include "Region.h"
#include "MapBase.h"
#include "WidgetComponent.h"
#include "AnalyticsManager.h"
#include "ProgressionManager.h"


UTutorialManager::UTutorialManager()
	: Super()
{
	PrimaryComponentTick.bCanEverTick = false;
	WorldIndicatorSize = FVector2D(500, 500);
}

void UTutorialManager::Init(APlayerController* InPlayerController, UWidgetComponent* InWidgetComponent)
{
	PlayerController = InPlayerController;
	TutorialWidgetComponent = InWidgetComponent;

	if (TutorialIndicatorWidget != nullptr)
	{
		TutorialWidget = CreateWidget<UUserWidget>(PlayerController, TutorialIndicatorWidget);
		TutorialWidget->SetVisibility(ESlateVisibility::Hidden);
		UPhoButton* TutorialWidgetButton = Cast<UPhoButton>(TutorialWidget->GetWidgetFromName(TutorialIndicatorButtonName));
		TutorialWidgetButton->OnClickedPho.AddDynamic(this, &UTutorialManager::OnTutorialIndicatorClicked);
		TutorialWidgetSlot = Cast<UCanvasPanelSlot>(TutorialWidgetButton->Slot);
	}

	if (TutorialDialogueWidgetClass != nullptr)
	{
		TutorialDialogueWidget = CreateWidget<UTutorialDialogueWidget>(PlayerController, TutorialDialogueWidgetClass);
		TutorialDialogueWidget->SetVisibility(ESlateVisibility::Hidden);
		UPhoButton* DialogueButton = Cast<UPhoButton>(TutorialDialogueWidget->GetWidgetFromName(TutorialDialogueButtonName));
		DialogueButton->OnClickedPho.AddDynamic(this, &UTutorialManager::OnTutorialDialoguePressed);
	}

	if (InterstitialWidgetClass != nullptr)
	{
		InterstitialWidget = CreateWidget<UUserWidget>(PlayerController, InterstitialWidgetClass);
		TutorialDialogueWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	TutorialWidgetComponent->SetDrawSize(WorldIndicatorSize);
	TutorialWidgetComponent->SetWidgetClass(WorldIndicatorWidgetClass);
	if (TutorialWidgetComponent->GetUserWidgetObject())
	{
		UPhoButton* WorldIndicatorButton = Cast<UPhoButton>(TutorialWidgetComponent->GetUserWidgetObject()->GetWidgetFromName(TutorialWorldButtonName));
		if (WorldIndicatorButton)
		{
			WorldIndicatorButton->OnClickedPho.AddDynamic(this, &UTutorialManager::OnWorldIndicatorPressed);
		}
		TutorialWidgetComponent->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Visible);
		TutorialWidgetComponent->SetVisibility(false);
	}

#if WITH_EDITOR
		FString TutorialAnalyticsProgression;
		UTutorialTemplate* TutorialItr = DefaultTutorial;
		while (TutorialItr != nullptr)
		{
			PlayerController->GetAnalyticsManager()->AppendTutorialEventString(TutorialAnalyticsProgression, TutorialItr->CatalogItemId, TutorialItr->GetStepNames());

			if (TutorialItr->CatalogCustomData.NextTutorial.Get() != nullptr)
			{
				TutorialItr = TutorialItr->CatalogCustomData.NextTutorial.Get<UTutorialTemplate>();
			}
			else
			{
				TutorialItr = nullptr;
			}
		}
		UE_LOG(Log, Display, TEXT("Tutorial Analytics Progression:\n%s"), *TutorialAnalyticsProgression);
#endif
}

void UTutorialManager::SetupDefaultTutorial()
{
	if (DefaultTutorial != nullptr)
	{
		TryStartTutorial(DefaultTutorial);
	}
}

void UTutorialManager::OnTutorialIndicatorClicked(UPhoButton* InButton)
{
	TutorialWidget->SetVisibility(ESlateVisibility::Hidden);
	InterstitialWidget->SetVisibility(ESlateVisibility::Visible);

	UWidget* CurrentTargetWidget = ActiveTutorial->GetCurrentTargetWidget();
	if (CurrentTargetWidget != nullptr)
	{
		// If applicable, pass through button press broadcasts to the widget indicated being indicated
		UButton* CurrentButton = Cast<UButton>(CurrentTargetWidget);
		UCloseWidget* CloseWidget = Cast<UCloseWidget>(CurrentTargetWidget);
		if (CloseWidget)
		{
			CloseWidget->OnClose.ExecuteIfBound();
		}
		else if (CurrentButton != nullptr)
		{
			CurrentButton->OnClicked.Broadcast();
		}

		if (ActiveTutorial->IsMenuUnchanged())
		{
			ScheduleTutorialAdvancement();
		}
		else if(ActiveTutorial->GetNextWidgetStepOverride() != nullptr)
		{
			PlayerController->GetHUD()->OpenMenuByClass(ActiveTutorial->GetNextWidgetStepOverride());
		}
	}
	else
	{
		EndTutorial();
	}
}

void UTutorialManager::TryStartTutorial(UTutorialTemplate* InTemplate) const
{
	if (!IsTutorialStarted(InTemplate))
	{
		PlayerController->GetInventoryComponent()->CreateItem<UTutorialItem>(InTemplate);
	}
}

void UTutorialManager::TryStartDynamicTutorial(const FGameplayTag& TutorialTag)
{
	if (!IsActive())
	{
		bool bHasDynamicTutorialTag = PlayerController->GetPlayerTags()->HasMatchingGameplayTag(TutorialTag);
		UTutorialItem* TutorialItem = GetActiveDynamicTutorial(TutorialTag);
		if (bHasDynamicTutorialTag && TutorialItem != nullptr)
		{
			SetActiveTutorial(TutorialItem);
		}
		else if(!bHasDynamicTutorialTag)
		{
			UTutorialTemplate* TutorialTemplate = GetDynamicTutorialTemplate(TutorialTag);
			if (TutorialTemplate != nullptr)
			{
				TryStartTutorial(TutorialTemplate);
			}
		}
	}
}

void UTutorialManager::ForceTutorialEnd()
{
	if (ActiveTutorial != nullptr)
	{
		// Iterate through remaining tutorials to apply any remaining effects that might effect gameplay
		UTutorialTemplate* ActiveTutorialTemplate = ActiveTutorial->GetItemTemplate<UTutorialTemplate>();
		while (ActiveTutorialTemplate != nullptr)
		{
			if (ActiveTutorialTemplate->bCustomBaseSetup && ActiveTutorialTemplate->RegionSettings.Num() > 0)
			{
				UTownManager* TownManager = PlayerController->GetTownManager();
				TownManager->ApplyTutorialBuildingSettings(ActiveTutorialTemplate->RegionSettings);
			}

			UPlayFabInventoryComponent* InventoryComponent = PlayerController->GetInventoryComponent();
			for (const auto& CatalogRef : ActiveTutorialTemplate->TutorialItemsGranted)
			{
				for (int32 i = 0; i < CatalogRef.Value; ++i)
				{
					InventoryComponent->CreateItem<UItem>(CatalogRef.Key);
				}
			}
			
			const TArray<FTutorialSequenceStep>& TutorialSequence = ActiveTutorialTemplate->TutorialSequence.SequenceSteps;
			for (const FTutorialSequenceStep& Step : TutorialSequence)
			{
				PlayerController->GetPlayerTags()->AddTag(Step.StepTag);
			}

			PlayerController->GetPlayerTags()->AddTag(ActiveTutorialTemplate->TutorialTag);

			UItemTemplate* NextTemplate = ActiveTutorialTemplate->CatalogCustomData.NextTutorial.Get();
			ActiveTutorialTemplate = Cast<UTutorialTemplate>(NextTemplate);
		}

		PlayerController->GetInventoryComponent()->RemoveItem(ActiveTutorial);
		TutorialWidget->RemoveFromViewport();
		TutorialDialogueWidget->RemoveFromViewport();
		
		TutorialWidgetComponent->SetVisibility(false);
		OnWorldIndicatorHidden.Broadcast();

		ActiveTutorial = nullptr;
		PlayerController->Save();
	}
}

void UTutorialManager::AddTutorialItem(UTutorialItem* InTutorialItem)
{
	if (InTutorialItem->GetTutorialType() == ETutorialType::Dynamic)
	{
		ActiveDynamicTutorials.Add(InTutorialItem);
	}

	if (InTutorialItem->GetTutorialType() == ETutorialType::Initial || PlayerController->IsPlayFabDataInitialized())
	{
		SetActiveTutorial(InTutorialItem);
	}
}

void UTutorialManager::OnWorldIndicatorPressed(class UPhoButton* InButton)
{
	TutorialWidgetComponent->SetVisibility(false);
	InterstitialWidget->SetVisibility(ESlateVisibility::Visible);

	const FTutorialWorldIndicatorData& WorldIndicatorData = ActiveTutorial->GetCurrentWorldIndicatorData();
	AActor* TargetActor = GetFocusedWorldActor(WorldIndicatorData);
	TargetActor->NotifyActorOnInputTouchEnd(ETouchIndex::Touch1);
	OnWorldIndicatorHidden.Broadcast();

	if (!ActiveTutorial->DoesWorldIndicatorOpenMenu())
	{
		ScheduleTutorialAdvancement();
	}
}

void UTutorialManager::OnTutorialDialoguePressed(class UPhoButton* InButton)
{
	if (TutorialDialogueWidget->IsTextDisplaying())
	{
		TutorialDialogueWidget->SkipTextDisplay();
	}
	else
	{
		TutorialDialogueWidget->SetVisibility(ESlateVisibility::Hidden);
		InterstitialWidget->SetVisibility(ESlateVisibility::Visible);
		ScheduleTutorialAdvancement();
	}
}

void UTutorialManager::ScheduleTutorialAdvancement()
{
	// Waits a frame to try to advance the tutorial in case a widget's geometry is targeted & it needs to tick to be cached
	if (CanAdvanceTutorial())
	{
		bAdvancementScheduled = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTutorialManager::AdvanceTutorial);
	}
}

void UTutorialManager::AdvanceTutorial()
{
	bool bTutorialComplete = ActiveTutorial->HandleTutorialAdvanced();
	InterstitialWidget->SetVisibility(ESlateVisibility::Hidden);

	if (bTutorialComplete)
	{
		EndTutorial();
	}
	else
	{
		DisplayTutorialStep();
	}
}

void UTutorialManager::DisplayTutorialStep()
{
	const FTutorialSequenceStep& CurrentStep = ActiveTutorial->GetCurrentSequenceStep();

	if (CurrentStep.bDialogueDisplayed)
	{
		DisplayDialogue();
	}
	else
	{
		if (CurrentStep.IndicatorData.bWorldIndicator)
		{
			DisplayWorldIndicator();
		}
		else
		{
			DisplayIndicator();
		}
	}

	bAdvancementScheduled = false;
}

void UTutorialManager::DisplayIndicator()
{
	const UWidget* TargetWidget = ActiveTutorial->GetCurrentTargetWidget();
	if (TargetWidget != nullptr)
	{
		PositionIndicatorOverWidget(TargetWidget);

		// Assign Style to tutorial widget button in order to force target button pressed sound onto the tutorial widget
		UPhoButton* TutorialWidgetButton = Cast<UPhoButton>(TutorialWidget->GetWidgetFromName(TutorialIndicatorButtonName));
		FButtonStyle NewStyle = TutorialWidgetButton->WidgetStyle;
		const UButton* TargetButton = Cast<UButton>(TargetWidget);
		if (TargetButton)
		{
			const FSlateSound& TargetPressedSound = TargetButton->WidgetStyle.PressedSlateSound;
			NewStyle.SetPressedSound(TargetPressedSound.GetResourceObject() != nullptr ? TargetPressedSound : FSlateSound());
		}
		else
		{
			NewStyle.SetPressedSound(FSlateSound());
		}
		TutorialWidgetButton->SetStyle(NewStyle);

		TutorialWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		InterstitialWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTutorialManager::DisplayWorldIndicator()
{
	const FTutorialWorldIndicatorData& WorldIndicatorData = ActiveTutorial->GetCurrentWorldIndicatorData();
	AActor* TargetActor = GetFocusedWorldActor(WorldIndicatorData);

	if (TargetActor)
	{
		PlayerController->MoveCameraToActor(TargetActor, !WorldIndicatorData.bMapIndicator);
		PositionIndicatorOverWorldPosition(TargetActor->GetActorLocation());
		OnWorldIndicatorDisplayed.Broadcast();
	}
	else
	{
		InterstitialWidget->SetVisibility(ESlateVisibility::Visible);
		ActiveTutorial->LogInvalidWorldIndicatorStep();
	}
}

void UTutorialManager::DisplayDialogue()
{
	TutorialDialogueWidget->SetDialogueData(ActiveTutorial->GetCurrentDialogueData());
	TutorialDialogueWidget->SetVisibility(ESlateVisibility::Visible);
}

void UTutorialManager::PositionIndicatorOverWidget(const UWidget* InWidget)
{
	// Get The Cached Geometry of the target widget in order to get the absolute position of it on screen
	const FGeometry& WidgetGeometry = InWidget->GetCachedGeometry();
	FVector2D AbsolutePosition = WidgetGeometry.GetAbsolutePosition();
	FVector2D AbsoluteSize = WidgetGeometry.GetAbsolutePositionAtCoordinates(FVector2D::UnitVector);
	const FVector2D& EdgeOffset = TutorialIndicatorEdgeOffset;

	if (FMath::IsNearlyZero(AbsolutePosition.SizeSquared()) && FMath::IsNearlyZero(AbsoluteSize.SizeSquared()))
	{
		ActiveTutorial->LogInvalidGraphicsStep();
	}

	FVector2D AbsolutePixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), AbsolutePosition, AbsolutePixelPosition, ViewportPosition);

	ViewportPosition += EdgeOffset;
	TutorialWidgetSlot->SetPosition(ViewportPosition);

	// Get the Viewport Size & remove the DPI scaling
	int32 ViewportX, ViewportY;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	ViewportX /= UWidgetLayoutLibrary::GetViewportScale(PlayerController);
	ViewportY /= UWidgetLayoutLibrary::GetViewportScale(PlayerController);

	// Get the "Size" (bottom & right offsets)
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), AbsoluteSize, AbsolutePixelPosition, ViewportPosition);

	FVector2D WidgetSize = FVector2D(ViewportX - ViewportPosition.X, ViewportY - ViewportPosition.Y);
	WidgetSize += EdgeOffset;
	TutorialWidgetSlot->SetSize(WidgetSize);
}

void UTutorialManager::PositionIndicatorOverWorldPosition(const FVector& InPosition)
{
	TutorialWidgetComponent->SetVisibility(true);
	TutorialWidgetComponent->SetWorldLocation(InPosition);
}

AActor* UTutorialManager::GetFocusedWorldActor(const FTutorialWorldIndicatorData& WorldIndicatorData) const
{
	if (WorldIndicatorData.bMapIndicator)
	{
		return PlayerController->GetMap()->GetTileAt(WorldIndicatorData.TileCoordinate);
	}
	else
	{
		ARegion* Region = PlayerController->GetTownManager()->GetRegion(WorldIndicatorData.RegionSlot);
		if (WorldIndicatorData.bSelectRegion)
		{
			return Region;
		}
		else
		{
			return Region->GetBuildingAtSlot(WorldIndicatorData.BuildingSlot);
		}
	}

	return nullptr;
}

void UTutorialManager::EndTutorial()
{
	bAdvancementScheduled = false;

	ActiveTutorial->EndTutorial();

	PlayerController->GetInventoryComponent()->RemoveItem(ActiveTutorial);

	TutorialWidget->RemoveFromViewport();
	TutorialDialogueWidget->RemoveFromViewport();
	InterstitialWidget->RemoveFromViewport();

	PlayerController->GetPlayerTags()->AddTag(ActiveTutorial->GetTutorialCompletionTag());

	if (ActiveTutorial->GetTutorialType() == ETutorialType::Dynamic)
	{
		ActiveDynamicTutorials.Remove(ActiveTutorial);
	}

	UTutorialItem* LastTutorial = ActiveTutorial;
	ActiveTutorial = nullptr;
	if (LastTutorial->GetNextTutorial().Get() != nullptr)
	{
		PlayerController->GetInventoryComponent()->CreateItem<UTutorialItem>(LastTutorial->GetNextTutorial());
	}
	else
	{
		PlayerController->OnTutorialEnded();
		PlayerController->Save();
	}
}

UTutorialTemplate* UTutorialManager::GetLastTutorialTemplate() const
{
	UTutorialTemplate* TemplateItr = DefaultTutorial;
	while (TemplateItr->CatalogCustomData.NextTutorial.Get() != nullptr)
	{
		TemplateItr = TemplateItr->CatalogCustomData.NextTutorial.Get<UTutorialTemplate>();
	}
	return TemplateItr;
}

UTutorialItem* UTutorialManager::GetActiveDynamicTutorial(const FGameplayTag& InTutorialTag)
{
	if (ActiveDynamicTutorials.Num() > 0)
	{
		auto ActiveDynamicTutorialPredicate = [InTutorialTag](UTutorialItem* Tutorial) {
			return Tutorial->GetItemTemplate<UTutorialTemplate>()->TutorialTag == InTutorialTag;
		};

		auto MatchingTutorial = ActiveDynamicTutorials.FindByPredicate(ActiveDynamicTutorialPredicate);
		if (MatchingTutorial != nullptr)
		{
			return *MatchingTutorial;
		}
	}
	return nullptr;
}

UTutorialTemplate* UTutorialManager::GetDynamicTutorialTemplate(const FGameplayTag& InTutorialTag) const
{
	if (DynamicTutorials.Num() > 0)
	{
		auto DynamicTutorialPredicate = [InTutorialTag](UTutorialTemplate* Tutorial) {
			return Tutorial->TutorialTag == InTutorialTag;
		};

		auto MatchingTutorial = DynamicTutorials.FindByPredicate(DynamicTutorialPredicate);
		if (MatchingTutorial != nullptr)
		{
			return *MatchingTutorial;
		}
	}

	return nullptr;
}

bool UTutorialManager::CanAdvanceTutorial() const
{
	return !bAdvancementScheduled && ActiveTutorial;
}

bool UTutorialManager::IsActive() const
{
	return ActiveTutorial != nullptr;
}

bool UTutorialManager::IsTutorialStarted(const UTutorialTemplate* InTemplate) const
{
	return PlayerController->GetPlayerTags()->HasMatchingGameplayTag(InTemplate->TutorialTag);
}

void UTutorialManager::SetActiveTutorial(class UTutorialItem* InTutorialItem)
{
	if (PlayerController->IsPlayFabDataInitialized())
	{
		PlayerController->GetProgressionManager()->RefreshMissionProgression();
		PlayerController->Save();
	}

	ActiveTutorial = InTutorialItem;

	TutorialWidget->AddToViewport(TutorialWidgetZOrder);
	TutorialDialogueWidget->AddToViewport(TutorialWidgetZOrder);

	InterstitialWidget->AddToViewport(TutorialWidgetZOrder);
	InterstitialWidget->SetVisibility(ESlateVisibility::Hidden);

	DisplayTutorialStep();

	PlayerController->OnTutorialStarted();
}
