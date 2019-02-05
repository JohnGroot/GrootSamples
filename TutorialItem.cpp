// Copyright 2018 Phosphor Studios. All Rights Reserved.

#include "TutorialItem.h"
#include "TutorialTemplate.h"
#include "PlayerController.h"
#include "HUDBase.h"
#include "HUDWidget.h"
#include "TutorialManager.h"
#include "Widget.h"
#include "TownManager.h"
#include "PlayerProfileTags.h"
#include "AnalyticsManager.h"
#include "PlayerProfileStats.h"

void UTutorialItem::PostCreateInitialize()
{
	if (PlayerController->IsPlayFabDataInitialized())
	{
		OnDataInitialized();
	}
	else
	{
		PlayerController->OnDataInitialized.AddUObject(this, &UTutorialItem::OnDataInitialized);
	}
}

void UTutorialItem::PostLoadInitialize()
{
	TutorialManager = PlayerController->GetTutorialManager();

	TutorialManager->AddTutorialItem(this);
}

UWidget* UTutorialItem::GetCurrentTargetWidget() const
{
	UWidget* OutWidget = nullptr;
	AHUDBase* HUD = PlayerController->GetHUD();
	const FTutorialSequence& CurrentTutorialSequence = GetTutorialTemplate()->TutorialSequence;
	const FTutorialSequenceStep& CurrentSequenceStep = CurrentTutorialSequence.SequenceSteps[StepIndex];
	const TArray<FName>& CurrentWidgetPath = CurrentSequenceStep.IndicatorData.WidgetData.TargetWidgetPath;

	if (HUD->IsPopupOpen())
	{
		OutWidget = HUD->GetCurrentPopup()->GetWidget<UWidget>(CurrentWidgetPath);
	}
	else if (HUD->IsMenuOpen())
	{
		OutWidget = HUD->GetCurrentMenu()->GetWidget<UWidget>(CurrentWidgetPath);
	}
	else
	{
		OutWidget = HUD->GetHudWidget()->GetWidget<UWidget>(CurrentWidgetPath);
	}

	if (OutWidget == nullptr)
	{
		UE_LOG(Log, Warning, TEXT("Unable to Get Current Tutorial Widget in Sequence named %s in Step Index %i named %s"),
			*CurrentTutorialSequence.SequenceName.ToString(), StepIndex, *CurrentTutorialSequence.SequenceSteps[StepIndex].SequenceStepName.ToString());
	}

	return OutWidget;
}

const FTutorialWorldIndicatorData& UTutorialItem::GetCurrentWorldIndicatorData() const
{
	return GetCurrentSequenceStep().IndicatorData.WorldIndicatorData;
}

const FTutorialDialogueData& UTutorialItem::GetCurrentDialogueData() const
{
	const FTutorialSequenceStep& CurrentStep = GetCurrentSequenceStep();
	return CurrentStep.DialogueData;
}

void UTutorialItem::EndTutorial()
{
	GetAnalytics()->OnTutorialEnd(this);
}

bool UTutorialItem::HandleTutorialAdvanced()
{
	GetAnalytics()->OnTutorialAdvanced(this);

	bool bTutorialComplete = StepIndex == GetStepCount() - 1;
	if (bTutorialComplete && GetTutorialTemplate()->CatalogCustomData.bCloseMenuOnCompletion)
	{
		PlayerController->GetHUD()->CloseCurrentMenu();
	}
	else if(!bTutorialComplete)
	{
		++StepIndex;

		ApplyStepEffects();
	}

	return bTutorialComplete;
}

void UTutorialItem::OnDataInitialized()
{
	GetAnalytics()->OnTutorialBegin(this);

	UTutorialTemplate* TutorialTemplate = GetTutorialTemplate();
	if (TutorialTemplate->bCustomBaseSetup && TutorialTemplate->RegionSettings.Num() > 0)
	{
		UTownManager* TownManager = PlayerController->GetTownManager();
		TownManager->ApplyTutorialBuildingSettings(GetTutorialTemplate()->RegionSettings);
	}

	for (const auto& CatalogRef : GetTutorialTemplate()->TutorialItemsGranted)
	{
		for (int32 i = 0; i < CatalogRef.Value; ++i)
		{
			GetInventory()->CreateItem<UItem>(CatalogRef.Key);
		}
	}

	PlayerController->GetPlayerTags()->AddTag(TutorialTemplate->TutorialTag);

	ApplyStepEffects();
}

bool UTutorialItem::IsMenuUnchanged() const
{
	return GetCurrentSequenceStep().IndicatorData.WidgetData.bMenuUnchangedOnClick;
}

bool UTutorialItem::DoesWorldIndicatorOpenMenu() const
{
	return GetCurrentSequenceStep().IndicatorData.WorldIndicatorData.bOpensMenu;
}

TSubclassOf<class UWidget> UTutorialItem::GetNextWidgetStepOverride() const
{
	return GetCurrentSequenceStep().IndicatorData.WidgetData.NextStepWidgetOverride;
}

const FGameplayTag& UTutorialItem::GetTutorialCompletionTag() const
{
	return GetTutorialTemplate()->TutorialCompletionTag;
}

const FCatalogReference& UTutorialItem::GetNextTutorial() const
{
	return GetTutorialTemplate()->CatalogCustomData.NextTutorial;
}

void UTutorialItem::ApplyStepEffects()
{
	const FTutorialSequenceStep& CurrentStep = GetCurrentSequenceStep();
	PlayerController->GetPlayerStats()->AddStatModifiers(CurrentStep.StepStatEffect);
	PlayerController->GetPlayerTags()->AddTag(CurrentStep.StepTag);
}

const FTutorialSequence& UTutorialItem::GetCurrentSequence() const
{
	return GetTutorialTemplate()->TutorialSequence;
}

int32 UTutorialItem::GetStepCount() const
{
	return GetTutorialTemplate()->TutorialSequence.SequenceSteps.Num();
}

const FTutorialSequenceStep& UTutorialItem::GetCurrentSequenceStep() const
{
	return GetTutorialTemplate()->TutorialSequence.SequenceSteps[StepIndex];
}

UTutorialTemplate* UTutorialItem::GetTutorialTemplate() const
{
	return GetItemTemplate<UTutorialTemplate>();
}

UAnalyticsManager* UTutorialItem::GetAnalytics() const
{
	return PlayerController->GetAnalyticsManager();
}

ETutorialType UTutorialItem::GetTutorialType() const
{
	return GetTutorialTemplate()->GetTutorialType();
}

void UTutorialItem::LogInvalidGraphicsStep() const
{
	const FTutorialSequence& CurrentTutorialSequence = GetCurrentSequence();
	UE_LOG(Log, Warning, TEXT("Current Tutorial Widget Target in Sequence named %s in Step Index %i named %s didn't have valid geometry to match"),
		*CurrentTutorialSequence.SequenceName.ToString(), StepIndex, *CurrentTutorialSequence.SequenceSteps[StepIndex].SequenceStepName.ToString());
}

void UTutorialItem::LogInvalidWorldIndicatorStep() const
{
	const FTutorialSequence& CurrentTutorialSequence = GetCurrentSequence();
	UE_LOG(Log, Warning, TEXT("Current Tutorial World Target in Sequence named %s in Step Index %i named %s wasn't valid"),
		*CurrentTutorialSequence.SequenceName.ToString(), StepIndex, *CurrentTutorialSequence.SequenceSteps[StepIndex].SequenceStepName.ToString());
}
