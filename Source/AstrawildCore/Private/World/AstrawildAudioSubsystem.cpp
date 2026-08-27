#include "World/AstrawildAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

namespace AstrawildAudioDefaults
{
    static TSoftObjectPtr<USoundBase> SoftSound(const TCHAR* ObjectPath)
    {
        return TSoftObjectPtr<USoundBase>(FSoftObjectPath(ObjectPath));
    }

    static FAstrawildAudioCueBinding MakeBinding(const TCHAR* CueId, const TCHAR* Tag, const TCHAR* AssetPath, const TCHAR* BiomeId = TEXT(""), const bool bIsNight = false)
    {
        FAstrawildAudioCueBinding Binding;
        Binding.CueId = FName(CueId);
        Binding.CueTag = FGameplayTag::RequestGameplayTag(FName(Tag), false);
        Binding.Sound = SoftSound(AssetPath);
        Binding.BiomeId = FName(BiomeId);
        Binding.bIsNight = bIsNight;
        return Binding;
    }
}

void UAstrawildAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PopulateDefaultRegistry();
}

void UAstrawildAudioSubsystem::Deinitialize()
{
    StopComponent(AmbientComponent, 0.0f);
    StopComponent(BattleComponent, 0.0f);
    AmbientComponent = nullptr;
    BattleComponent = nullptr;
    Super::Deinitialize();
}

void UAstrawildAudioSubsystem::PopulateDefaultRegistry()
{
    if (AmbientCues.IsEmpty())
    {
        AmbientCues = {
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.DawnMeadows.Day"), TEXT("Audio.Ambient.DawnMeadows.Day"), TEXT("/Game/Astrawild/Audio/Ambience/SC_DawnMeadows_Day.SC_DawnMeadows_Day"), TEXT("Biome.DawnMeadows")),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.DawnMeadows.Night"), TEXT("Audio.Ambient.DawnMeadows.Night"), TEXT("/Game/Astrawild/Audio/Ambience/SC_DawnMeadows_Night.SC_DawnMeadows_Night"), TEXT("Biome.DawnMeadows"), true),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.SylvanRainforest.Day"), TEXT("Audio.Ambient.SylvanRainforest.Day"), TEXT("/Game/Astrawild/Audio/Ambience/SC_SylvanRainforest_Day.SC_SylvanRainforest_Day"), TEXT("Biome.SylvanRainforest")),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.SylvanRainforest.Night"), TEXT("Audio.Ambient.SylvanRainforest.Night"), TEXT("/Game/Astrawild/Audio/Ambience/SC_SylvanRainforest_Night.SC_SylvanRainforest_Night"), TEXT("Biome.SylvanRainforest"), true),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.ScorchedObsidianCaldera.Day"), TEXT("Audio.Ambient.ScorchedObsidianCaldera.Day"), TEXT("/Game/Astrawild/Audio/Ambience/SC_Caldera_Day.SC_Caldera_Day"), TEXT("Biome.ScorchedObsidianCaldera")),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.ScorchedObsidianCaldera.Night"), TEXT("Audio.Ambient.ScorchedObsidianCaldera.Night"), TEXT("/Game/Astrawild/Audio/Ambience/SC_Caldera_Night.SC_Caldera_Night"), TEXT("Biome.ScorchedObsidianCaldera"), true),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.GlacialZenith.Day"), TEXT("Audio.Ambient.GlacialZenith.Day"), TEXT("/Game/Astrawild/Audio/Ambience/SC_GlacialZenith_Day.SC_GlacialZenith_Day"), TEXT("Biome.GlacialZenith")),
            AstrawildAudioDefaults::MakeBinding(TEXT("Ambient.GlacialZenith.Night"), TEXT("Audio.Ambient.GlacialZenith.Night"), TEXT("/Game/Astrawild/Audio/Ambience/SC_GlacialZenith_Night.SC_GlacialZenith_Night"), TEXT("Biome.GlacialZenith"), true)
        };
    }

    if (BossThemes.IsEmpty())
    {
        const auto MakeTheme = [](const TCHAR* EncounterId, const TCHAR* Prefix, const TCHAR* TagPrefix) -> FAstrawildBossAudioTheme
        {
            FAstrawildBossAudioTheme Theme;
            Theme.EncounterId = FName(EncounterId);
            const FString PhaseOneId = FString::Printf(TEXT("%s.PhaseOne"), Prefix);
            const FString PhaseTwoId = FString::Printf(TEXT("%s.PhaseTwo"), Prefix);
            const FString UltimateId = FString::Printf(TEXT("%s.Ultimate"), Prefix);
            const FString SafePrefix = FString(Prefix).Replace(TEXT("."), TEXT("_"));
            const FString PhaseOnePath = FString::Printf(TEXT("/Game/Astrawild/Audio/Boss/%s_PhaseOne.%s_PhaseOne"), *SafePrefix, *SafePrefix);
            const FString PhaseTwoPath = FString::Printf(TEXT("/Game/Astrawild/Audio/Boss/%s_PhaseTwo.%s_PhaseTwo"), *SafePrefix, *SafePrefix);
            const FString UltimatePath = FString::Printf(TEXT("/Game/Astrawild/Audio/Boss/%s_Ultimate.%s_Ultimate"), *SafePrefix, *SafePrefix);
            Theme.PhaseOne = AstrawildAudioDefaults::MakeBinding(*PhaseOneId, *FString::Printf(TEXT("%s.PhaseOne"), TagPrefix), *PhaseOnePath);
            Theme.PhaseTwo = AstrawildAudioDefaults::MakeBinding(*PhaseTwoId, *FString::Printf(TEXT("%s.PhaseTwo"), TagPrefix), *PhaseTwoPath);
            Theme.Ultimate = AstrawildAudioDefaults::MakeBinding(*UltimateId, *FString::Printf(TEXT("%s.Ultimate"), TagPrefix), *UltimatePath);
            return Theme;
        };
        BossThemes = {
            MakeTheme(TEXT("Tower.SolarSpire"), TEXT("Boss.SolarixAlpha"), TEXT("Audio.Boss.SolarixAlpha")),
            MakeTheme(TEXT("Tower.TorrentSpire"), TEXT("Boss.Miremaw"), TEXT("Audio.Boss.Miremaw")),
            MakeTheme(TEXT("Tower.GeoSpire"), TEXT("Boss.Terradon"), TEXT("Audio.Boss.Terradon")),
            MakeTheme(TEXT("Tower.VoltSpire"), TEXT("Boss.Stormshell"), TEXT("Audio.Boss.Stormshell")),
            MakeTheme(TEXT("Tower.AbyssalSpire"), TEXT("Boss.FirstDawnDragon"), TEXT("Audio.Boss.FirstDawnDragon"))
        };
    }
}

bool UAstrawildAudioSubsystem::PlayAmbientForBiome(const FName BiomeId, const bool bIsNight)
{
    const FAstrawildAudioCueBinding* Binding = FindAmbientCue(BiomeId, bIsNight);
    if (!Binding)
    {
        OnAudioFallback.Broadcast(FName(*FString::Printf(TEXT("Ambient.%s.%s"), *BiomeId.ToString(), bIsNight ? TEXT("Night") : TEXT("Day"))));
        return false;
    }
    return PlayCue(*Binding, EAstrawildAudioMode::Ambient, 1.5f, true);
}

bool UAstrawildAudioSubsystem::EnterBossCombat(const FName EncounterId, const int32 PhaseIndex, const bool bUltimate)
{
    const FAstrawildBossAudioTheme* Theme = FindBossTheme(EncounterId);
    if (!Theme)
    {
        OnAudioFallback.Broadcast(EncounterId);
        return false;
    }
    const FAstrawildAudioCueBinding* Binding = bUltimate ? &Theme->Ultimate : (PhaseIndex >= 2 ? &Theme->PhaseTwo : &Theme->PhaseOne);
    const EAstrawildAudioMode Mode = bUltimate ? EAstrawildAudioMode::CombatUltimate : (PhaseIndex >= 2 ? EAstrawildAudioMode::CombatPhaseTwo : EAstrawildAudioMode::CombatPhaseOne);
    if (!PlayCue(*Binding, Mode, 0.75f, true))
    {
        return false;
    }
    CurrentEncounterId = EncounterId;
    return true;
}

void UAstrawildAudioSubsystem::ExitCombat(const float FadeOutSeconds)
{
    StopComponent(BattleComponent, FadeOutSeconds);
    CurrentEncounterId = NAME_None;
    CurrentCueId = NAME_None;
    CurrentMode = EAstrawildAudioMode::Ambient;
    OnAudioModeChanged.Broadcast(CurrentMode, CurrentCueId);
}

bool UAstrawildAudioSubsystem::HasPlayableCue(const FName CueId) const
{
    for (const FAstrawildAudioCueBinding& Binding : AmbientCues)
    {
        if (Binding.CueId == CueId)
        {
            return !Binding.Sound.IsNull();
        }
    }
    for (const FAstrawildBossAudioTheme& Theme : BossThemes)
    {
        for (const FAstrawildAudioCueBinding* Binding : {&Theme.PhaseOne, &Theme.PhaseTwo, &Theme.Ultimate})
        {
            if (Binding->CueId == CueId)
            {
                return !Binding->Sound.IsNull();
            }
        }
    }
    return false;
}

bool UAstrawildAudioSubsystem::IsInCombat() const
{
    return CurrentMode != EAstrawildAudioMode::Ambient;
}

bool UAstrawildAudioSubsystem::PlayCue(const FAstrawildAudioCueBinding& Binding, const EAstrawildAudioMode Mode, const float FadeInSeconds, const bool bLoop)
{
    if (!GetWorld())
    {
        return false;
    }
    USoundBase* Sound = Binding.Sound.LoadSynchronous();
    if (!Sound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ASTRAWILD][Audio] Missing sound asset for cue %s; Editor must create/import the registered asset."), *Binding.CueId.ToString());
        OnAudioFallback.Broadcast(Binding.CueId);
        return false;
    }

    TObjectPtr<UAudioComponent>& Component = Mode == EAstrawildAudioMode::Ambient ? AmbientComponent : BattleComponent;
    StopComponent(Component, 0.35f);
    Component = NewObject<UAudioComponent>(GetWorld());
    Component->bAutoActivate = false;
    Component->bIsUISound = false;
    Component->SetSound(Sound);
    // Looping is authored on the Sound Cue/Wave; bLoop is retained as a registry contract.
    (void)bLoop;
    Component->RegisterComponentWithWorld(GetWorld());
    Component->FadeIn(FadeInSeconds, FMath::Max(0.0f, Binding.VolumeMultiplier), 0.0f, EAudioFaderCurve::SCurve);
    Component->SetPitchMultiplier(FMath::Max(0.1f, Binding.PitchMultiplier));
    CurrentMode = Mode;
    CurrentCueId = Binding.CueId;
    OnAudioModeChanged.Broadcast(CurrentMode, CurrentCueId);
    return true;
}

void UAstrawildAudioSubsystem::StopComponent(TObjectPtr<UAudioComponent>& Component, const float FadeOutSeconds)
{
    if (!Component)
    {
        return;
    }
    if (FadeOutSeconds > 0.0f)
    {
        Component->FadeOut(FadeOutSeconds, 0.0f);
    }
    else
    {
        Component->Stop();
    }
    Component->DestroyComponent();
    Component = nullptr;
}

const FAstrawildAudioCueBinding* UAstrawildAudioSubsystem::FindAmbientCue(const FName BiomeId, const bool bIsNight) const
{
    for (const FAstrawildAudioCueBinding& Binding : AmbientCues)
    {
        if (Binding.BiomeId == BiomeId && Binding.bIsNight == bIsNight)
        {
            return &Binding;
        }
    }
    return nullptr;
}

const FAstrawildBossAudioTheme* UAstrawildAudioSubsystem::FindBossTheme(const FName EncounterId) const
{
    return BossThemes.FindByPredicate([EncounterId](const FAstrawildBossAudioTheme& Theme)
    {
        return Theme.EncounterId == EncounterId;
    });
}
