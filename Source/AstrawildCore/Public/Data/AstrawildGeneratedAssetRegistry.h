#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "AstrawildGeneratedAssetRegistry.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGeneratedMeshBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    FName AssetId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    FName Consumer = NAME_None;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGeneratedAudioBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    FName CueId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    TSoftObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generated Asset")
    FName Consumer = NAME_None;
};

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildGeneratedAssetRegistry : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Generated Assets")
    TArray<FAstrawildGeneratedMeshBinding> Meshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Generated Assets")
    TArray<FAstrawildGeneratedAudioBinding> Audio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Generated Assets")
    FName RegistryVersion = TEXT("GeneratedAssets.v1");

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Generated Assets")
    bool TryGetMesh(FName AssetId, FAstrawildGeneratedMeshBinding& OutBinding) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Generated Assets")
    bool TryGetAudio(FName CueId, FAstrawildGeneratedAudioBinding& OutBinding) const;

    const FAstrawildGeneratedMeshBinding* FindMesh(FName AssetId) const;
    const FAstrawildGeneratedAudioBinding* FindAudio(FName CueId) const;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(TEXT("AstrawildGeneratedAssets"), GetFName());
    }
};
