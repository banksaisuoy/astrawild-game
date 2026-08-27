#include "Data/AstrawildGeneratedAssetRegistry.h"

bool UAstrawildGeneratedAssetRegistry::TryGetMesh(const FName AssetId, FAstrawildGeneratedMeshBinding& OutBinding) const
{
    const FAstrawildGeneratedMeshBinding* Found = FindMesh(AssetId);
    if (!Found)
    {
        OutBinding = FAstrawildGeneratedMeshBinding();
        return false;
    }
    OutBinding = *Found;
    return true;
}

bool UAstrawildGeneratedAssetRegistry::TryGetAudio(const FName CueId, FAstrawildGeneratedAudioBinding& OutBinding) const
{
    const FAstrawildGeneratedAudioBinding* Found = FindAudio(CueId);
    if (!Found)
    {
        OutBinding = FAstrawildGeneratedAudioBinding();
        return false;
    }
    OutBinding = *Found;
    return true;
}

const FAstrawildGeneratedMeshBinding* UAstrawildGeneratedAssetRegistry::FindMesh(const FName AssetId) const
{
    return Meshes.FindByPredicate([AssetId](const FAstrawildGeneratedMeshBinding& Binding)
    {
        return Binding.AssetId == AssetId;
    });
}

const FAstrawildGeneratedAudioBinding* UAstrawildGeneratedAssetRegistry::FindAudio(const FName CueId) const
{
    return Audio.FindByPredicate([CueId](const FAstrawildGeneratedAudioBinding& Binding)
    {
        return Binding.CueId == CueId;
    });
}
