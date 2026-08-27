#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Sound/SoundBase.h"
#include "AstrawildAudioSubsystem.generated.h"

class UAudioComponent;

UENUM(BlueprintType)
enum class EAstrawildAudioMode : uint8
{
    Ambient,
    CombatPhaseOne,
    CombatPhaseTwo,
    CombatUltimate
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildAudioCueBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
    FName CueId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Ambient")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Ambient")
    bool bIsNight = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
    FGameplayTag CueTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
    TSoftObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.0"))
    float VolumeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.1"))
    float PitchMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBossAudioTheme
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Boss")
    FName EncounterId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Boss")
    FAstrawildAudioCueBinding PhaseOne;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Boss")
    FAstrawildAudioCueBinding PhaseTwo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio|Boss")
    FAstrawildAudioCueBinding Ultimate;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildAudioModeChangedSignature, EAstrawildAudioMode, NewMode, FName, CueId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildAudioFallbackSignature, FName, CueId);

UCLASS()
class ASTRAWILDCORE_API UAstrawildAudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Audio|Ambient")
    TArray<FAstrawildAudioCueBinding> AmbientCues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Audio|Boss")
    TArray<FAstrawildBossAudioTheme> BossThemes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Audio")
    EAstrawildAudioMode CurrentMode = EAstrawildAudioMode::Ambient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Audio")
    FName CurrentCueId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Audio")
    FName CurrentEncounterId = NAME_None;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Audio|Events")
    FOnAstrawildAudioModeChangedSignature OnAudioModeChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Audio|Events")
    FOnAstrawildAudioFallbackSignature OnAudioFallback;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Audio")
    bool PlayAmbientForBiome(FName BiomeId, bool bIsNight = false);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Audio")
    bool EnterBossCombat(FName EncounterId, int32 PhaseIndex = 1, bool bUltimate = false);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Audio")
    void ExitCombat(float FadeOutSeconds = 1.0f);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Audio")
    bool HasPlayableCue(FName CueId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Audio")
    bool IsInCombat() const;

protected:
    void PopulateDefaultRegistry();
    bool PlayCue(const FAstrawildAudioCueBinding& Binding, EAstrawildAudioMode Mode, float FadeInSeconds, bool bLoop);
    void StopComponent(TObjectPtr<UAudioComponent>& Component, float FadeOutSeconds);
    const FAstrawildAudioCueBinding* FindAmbientCue(FName BiomeId, bool bIsNight) const;
    const FAstrawildBossAudioTheme* FindBossTheme(FName EncounterId) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AmbientComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> BattleComponent;
};
