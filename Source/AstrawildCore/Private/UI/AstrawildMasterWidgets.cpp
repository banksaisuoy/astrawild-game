#include "UI/AstrawildMasterWidgets.h"

#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildSanComponent.h"
#include "Components/AstrawildSurvivalComponent.h"
#include "Components/AstrawildTechnologyComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "World/AstrawildDungeonSubsystem.h"

void UAstrawildGameplayHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= 0.1f)
    {
        RefreshAccumulator = 0.0f;
        RefreshGameplayState();
    }
}

void UAstrawildGameplayHUDWidget::RefreshGameplayState()
{
    const APlayerController* PlayerController = GetOwningPlayer();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!PlayerPawn)
    {
        return;
    }

    if (const UAstrawildAttributeComponent* Attributes = PlayerPawn->FindComponentByClass<UAstrawildAttributeComponent>())
    {
        HealthPercent = Attributes->MaxHealth > 0.0f ? Attributes->CurrentHealth / Attributes->MaxHealth : 0.0f;
        StaminaPercent = Attributes->MaxStamina > 0.0f ? Attributes->CurrentStamina / Attributes->MaxStamina : 0.0f;
    }
    if (const UAstrawildSurvivalComponent* Survival = PlayerPawn->FindComponentByClass<UAstrawildSurvivalComponent>())
    {
        HungerPercent = Survival->MaxHunger > 0.0f ? Survival->CurrentHunger / Survival->MaxHunger : 0.0f;
        ThirstPercent = Survival->MaxThirst > 0.0f ? Survival->CurrentThirst / Survival->MaxThirst : 0.0f;
    }
    if (const UAstrawildSanComponent* San = PlayerPawn->FindComponentByClass<UAstrawildSanComponent>())
    {
        SANPercent = San->GetSANPercent();
    }
}

int32 UAstrawildEchoDexWidget::RefreshDex()
{
    VisibleEntries.Reset();
    if (!EchoDexTable)
    {
        return 0;
    }

    TArray<FAstrawildEchoDexRow*> Rows;
    EchoDexTable->GetAllRows<FAstrawildEchoDexRow>(TEXT("EchoDexWidget"), Rows);
    for (const FAstrawildEchoDexRow* Row : Rows)
    {
        if (Row)
        {
            VisibleEntries.Add(*Row);
        }
    }
    VisibleEntries.Sort([](const FAstrawildEchoDexRow& Left, const FAstrawildEchoDexRow& Right)
    {
        return Left.DexOrder < Right.DexOrder;
    });
    return VisibleEntries.Num();
}

bool UAstrawildEchoDexWidget::SelectDexOrder(const int32 DexOrder)
{
    for (const FAstrawildEchoDexRow& Entry : VisibleEntries)
    {
        if (Entry.DexOrder == DexOrder)
        {
            SelectedDexOrder = DexOrder;
            return true;
        }
    }
    SelectedDexOrder = INDEX_NONE;
    return false;
}

int32 UAstrawildTechnologyTreeWidget::RefreshTechnologyState()
{
    VisibleNodes.Reset();
    UnlockedTechnologyTags.Reset();
    ResearchPoints = 0;

    const APlayerController* PlayerController = GetOwningPlayer();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (const UAstrawildTechnologyComponent* Technology = PlayerPawn ? PlayerPawn->FindComponentByClass<UAstrawildTechnologyComponent>() : nullptr)
    {
        UnlockedTechnologyTags = Technology->UnlockedTechnologyTags;
        ResearchPoints = Technology->ResearchPoints;
    }
    if (!TechnologyTable)
    {
        return 0;
    }

    TArray<FAstrawildTechnologyNodeRow*> Rows;
    TechnologyTable->GetAllRows<FAstrawildTechnologyNodeRow>(TEXT("TechnologyWidget"), Rows);
    for (const FAstrawildTechnologyNodeRow* Row : Rows)
    {
        if (Row)
        {
            VisibleNodes.Add(*Row);
        }
    }
    VisibleNodes.Sort([](const FAstrawildTechnologyNodeRow& Left, const FAstrawildTechnologyNodeRow& Right)
    {
        return Left.Tier == Right.Tier ? Left.ResearchCost < Right.ResearchCost : Left.Tier < Right.Tier;
    });
    return VisibleNodes.Num();
}

void UAstrawildDungeonStatusWidget::RefreshDungeonState()
{
    ActiveDungeonId = NAME_None;
    RemainingTimeSeconds = 0.0f;
    ParticipantCount = 0;

    if (UWorld* World = GetWorld())
    {
        if (const UAstrawildDungeonSubsystem* Dungeon = World->GetSubsystem<UAstrawildDungeonSubsystem>())
        {
            ActiveDungeonId = Dungeon->ActiveDungeonId;
            RemainingTimeSeconds = Dungeon->GetRemainingTimeSeconds();
            ParticipantCount = Dungeon->GetParticipantCount();
        }
    }
}

void UAstrawildMasterHUDWidget::RefreshAllPanels()
{
    if (GameplayHUD)
    {
        GameplayHUD->RefreshGameplayState();
    }
    if (InventoryPanel)
    {
        InventoryPanel->RefreshInventory();
    }
    if (CraftingPanel)
    {
        CraftingPanel->RefreshRecipes();
    }
    if (EchoDexPanel)
    {
        EchoDexPanel->RefreshDex();
    }
    if (TechnologyPanel)
    {
        TechnologyPanel->RefreshTechnologyState();
    }
    if (DungeonPanel)
    {
        DungeonPanel->RefreshDungeonState();
    }
}
