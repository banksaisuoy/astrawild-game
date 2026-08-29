#include "AstrawildCheatManager.h"

#include "AstrawildCore.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

AAstrawildPlayerCharacter* UAstrawildCheatManager::GetPlayer() const
{
    const APlayerController* PC = Cast<APlayerController>(GetOuter());
    return PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
}

void UAstrawildCheatManager::SpawnEcho(const FName EchoDefinitionId)
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = GetPlayer();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildEchoDefinition* Definition = Registry ? Registry->FindEcho(EchoDefinitionId) : nullptr;

    if (!World || !Player || !Definition)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SpawnEcho: unknown Echo id %s."), *EchoDefinitionId.ToString());
        return;
    }

    const FVector Location = Player->GetActorLocation() + Player->GetActorForwardVector() * 400.0f;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params))
    {
        Echo->InitializeFromDefinition(Definition);
    }
}

void UAstrawildCheatManager::GiveItem(const FName ItemId, const int32 Quantity)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->AddItem(ItemId, FMath::Max(1, Quantity));
    }
}

void UAstrawildCheatManager::SetTime(const int32 Hour, const int32 Minute)
{
    UWorld* World = GetWorld();
    if (UAstrawildTimeSubsystem* Time = World ? World->GetSubsystem<UAstrawildTimeSubsystem>() : nullptr)
    {
        Time->SetTimeOfDay(Hour, Minute);
    }
}

void UAstrawildCheatManager::SetWeather(const FName WeatherName)
{
    UWorld* World = GetWorld();
    UAstrawildWeatherSubsystem* Weather = World ? World->GetSubsystem<UAstrawildWeatherSubsystem>() : nullptr;
    if (!Weather)
    {
        return;
    }

    const FString Name = WeatherName.ToString().ToLower();
    if (Name == TEXT("clear")) { Weather->ForceWeather(EAstrawildWeatherState::Clear); }
    else if (Name == TEXT("cloudy")) { Weather->ForceWeather(EAstrawildWeatherState::Cloudy); }
    else if (Name == TEXT("rain")) { Weather->ForceWeather(EAstrawildWeatherState::Rain); }
    else if (Name == TEXT("heavyrain")) { Weather->ForceWeather(EAstrawildWeatherState::HeavyRain); }
    else if (Name == TEXT("storm")) { Weather->ForceWeather(EAstrawildWeatherState::Storm); }
    else if (Name == TEXT("fog")) { Weather->ForceWeather(EAstrawildWeatherState::Fog); }
    else if (Name == TEXT("heat")) { Weather->ForceWeather(EAstrawildWeatherState::Heat); }
    else if (Name == TEXT("cold")) { Weather->ForceWeather(EAstrawildWeatherState::Cold); }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SetWeather: unknown weather %s (clear/cloudy/rain/heavyrain/storm/fog/heat/cold)."), *WeatherName.ToString());
    }
}

void UAstrawildCheatManager::God()
{
    if (AAstrawildPlayerCharacter* Player = GetPlayer())
    {
        if (Player->SurvivalComponent)
        {
            Player->SurvivalComponent->SetGodMode(!Player->SurvivalComponent->IsGodMode());
        }
    }
}

void UAstrawildCheatManager::HealAll()
{
    if (AAstrawildPlayerCharacter* Player = GetPlayer())
    {
        if (Player->SurvivalComponent)
        {
            Player->SurvivalComponent->FullRestore();
        }
    }
}

void UAstrawildCheatManager::ResearchPoints(const int32 Amount)
{
    const UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->AddResearchPoints(FMath::Max(0, Amount));
        }
    }
}

void UAstrawildCheatManager::UnlockTech(const FName TechId)
{
    const UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->TryUnlockTech(TechId);
        }
    }
}

void UAstrawildCheatManager::SaveNow()
{
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* Save = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            Save->SaveWorld(World);
        }
    }
}

void UAstrawildCheatManager::LoadNow()
{
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* Save = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            Save->LoadWorld(World);
        }
    }
}

void UAstrawildCheatManager::CaptureAll()
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (!World || !Player)
    {
        return;
    }

    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (Echo && !Echo->bCaptured && !Echo->IsDefeated() && IsValid(Echo->EchoDefinition))
        {
            Echo->OwnerPlayerId = Player->GetFName();
            Echo->Capture(50.0f);
        }
    }
}

void UAstrawildCheatManager::TeleportForward(const float Distance)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (Player)
    {
        const FVector Target = Player->GetActorLocation() + Player->GetActorForwardVector() * FMath::Max(0.0f, Distance);
        Player->SetActorLocation(Target + FVector(0, 0, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
    }
}
