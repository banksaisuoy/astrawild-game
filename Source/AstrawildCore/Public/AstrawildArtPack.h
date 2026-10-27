#pragma once

#include "CoreMinimal.h"

/**
 * ASTRAWILD Art Pack binding tables (Production V2 Batch 4 — Visual Vertical Slice).
 *
 * Single source of truth for every soft asset path the CODE_DEFAULT content
 * binds. The tables are PURE DATA (no engine object loads) so automation tests
 * can verify the contract without a world, and the dashboard/docs can render
 * the same values. Consumers:
 *   - UAstrawildProductionContent applies these onto definitions after registration.
 *   - UAstrawildContentLibrary::WarmArtPackBindings pre-loads every referenced asset
 *     (fallbacks stay active for any path that does not resolve yet).
 *   - AAstrawildPlayerCharacter / AAstrawildEchoCharacter / AAstrawildResourceNode
 *     consume the bound meshes + animation clips with PMC fallbacks.
 *
 * Paths mirror /ArtSource/manifest.json (the ArtSourceGen pipeline output map).
 * Art pack source assets + import pipeline: Tools/ArtSourceGen + Content/Python/AwPipeline.
 */
namespace AstrawildArtPack
{
        /** Weapon art binding (CP-03): held mesh + FX + sound soft paths. */
        struct FWeaponArt
        {
                FName WeaponId;
                FString MeshPath;
                FString MuzzleVfxPath;
                FString ImpactVfxPath;
                FString TrailVfxPath;
                FString FireSoundPath;
                FString ImpactSoundPath;
        };

        /** Echo art binding (CP-02/CP-08): skeletal mesh + locomotion clips. */
        struct FEchoArt
        {
                FName EchoId;
                FString MeshPath;
                FString IdleAnimPath;
                FString MoveAnimPath;
        };

        /** Biome dressing art binding (CP-04): scatter meshes + master material + ambience. */
        struct FBiomeArt
        {
                FName BiomeId;
                TArray<FString> TreeMeshPaths;
                TArray<FString> RockMeshPaths;
                TArray<FString> GrassMeshPaths;
                FString LandscapeMaterialPath;
                FString AmbientAudioPath;
        };

        /** Resource node art binding (CP-04): crystal/rock cluster mesh. */
        struct FNodeArt
        {
                FName NodeId;
                FString MeshPath;
        };

        /**
         * Landmark art binding (P-5/P-6 of the art plan): the CC0 ruin pillar
         * shared by resonance pillars (dungeons) and POI beacon markers (world).
         * Consumers bounds-fit the mesh at runtime and keep the engine-primitive
         * constructor stand-in as the fail-closed fallback (CP-00 rule 2).
         */
        struct FLandmarkArt
        {
                FString PillarMeshPath;
        };

        /** Survivor art binding (CP-01/CP-08): player skeletal mesh + 7 clips. */
        struct FSurvivorArt
        {
                FString MeshPath;
                FString IdleAnimPath;
                FString WalkAnimPath;
                FString RunAnimPath;
                FString JumpAnimPath;
                FString AimAnimPath;
                FString FireAnimPath;
                FString GatherAnimPath;
        };

        /** Shared hero Niagara system paths (authored in-editor per CP-05 + handoff names). */
        namespace Vfx
        {
                extern ASTRAWILDCORE_API const TCHAR* MuzzleFlash;
                extern ASTRAWILDCORE_API const TCHAR* ImpactBurst;
                extern ASTRAWILDCORE_API const TCHAR* ProjectileTrail;
        }

        /** Shared weapon audio paths (authored per CP-06 — see the Paths table in the cpp). */
        namespace Sfx
        {
                /**
                 * DP-5: the bound energy impact cue, reused as the weakness-hit
                 * feedback sound. Points at the EXISTING binding — no new
                 * /Game/ reference is introduced (validator check 8 stays clean).
                 */
                extern ASTRAWILDCORE_API const TCHAR* WeaknessHitImpact;

                /**
                 * CGC-3 (audit 2-b audio activation): the widely-shared cue paths
                 * promoted to named constants — every one resolves against the
                 * flat-imported Content/Audio packages (validator check 8), and
                 * each is consumed by MORE THAN ONE call site so the constant
                 * genuinely removes literal duplication:
                 *   - ChargeLoop: melee swing blip + boss melee windup + skiff hum
                 *     (honest reuse — the 1.6s cue IS an engine spool).
                 *   - ImpactKinetic: first-hit-in-swing melee impact + harvest node.
                 *   - UiClick: screen-open feedback + building placement.
                 *   - UiCancel: screen-close feedback + building dismantle.
                 */
                extern ASTRAWILDCORE_API const TCHAR* ChargeLoop;
                extern ASTRAWILDCORE_API const TCHAR* ImpactKinetic;
                extern ASTRAWILDCORE_API const TCHAR* UiClick;
                extern ASTRAWILDCORE_API const TCHAR* UiCancel;
        }

        /** All 8 production weapon bindings (may contain null paths when a weapon has no art yet). */
        ASTRAWILDCORE_API const TArray<FWeaponArt>& GetWeaponArt();

        /** All 6 production Echo bindings. */
        ASTRAWILDCORE_API const TArray<FEchoArt>& GetEchoArt();

        /** All 12 biome dressing bindings. */
        ASTRAWILDCORE_API const TArray<FBiomeArt>& GetBiomeArt();

        /** All 10 resource node bindings. */
        ASTRAWILDCORE_API const TArray<FNodeArt>& GetNodeArt();

        /** The shared landmark binding (ruin pillar for POI markers + resonance pillars). */
        ASTRAWILDCORE_API const FLandmarkArt& GetLandmarkArt();

        /**
         * GS-1 (asset-path hardening): canonicalize a package-style asset path
         * ("/Game/X/SM_A") into the full object-path form ("/Game/X/SM_A.SM_A").
         * FSoftObjectPath's canonical form requires the object-name suffix;
         * depending on engine version a package-style-only path parses to an
         * INVALID asset path and then silently fails to resolve (the art binding
         * falls back to primitives forever with no error). Paths that already
         * carry a suffix (or are empty) pass through untouched. Pure string data —
         * the automation suite pins the behavior without a world.
         */
        ASTRAWILDCORE_API FString CanonicalAssetPath(const FString& PackageStylePath);

        /** The survivor (player) binding. */
        ASTRAWILDCORE_API const FSurvivorArt& GetSurvivorArt();

        /** Lookup helpers (return nullptr when the id has no binding). */
        ASTRAWILDCORE_API const FWeaponArt* FindWeaponArt(const FName& WeaponId);
        ASTRAWILDCORE_API const FEchoArt* FindEchoArt(const FName& EchoId);
        ASTRAWILDCORE_API const FBiomeArt* FindBiomeArt(const FName& BiomeId);
        ASTRAWILDCORE_API const FNodeArt* FindNodeArt(const FName& NodeId);

        // --- PCR-4/PCR-5: Tier-B archetype mesh library (strategy §6) ---

        /**
         * The 39 Tier-B species whose variant-baked GLBs live in
         * ArtSource/Meshes/Echoes (baked by Tools/ArtSourceGen/gen_tier_b.py —
         * unique proportions/features/palette per species, never a plain
         * recolor). This list mirrors the ArtSource manifest: the automation
         * contract (ASTRAWILD.PCR4.TierBLibrary) and the static validator both
         * verify every entry has a GLB on disk.
         */
        ASTRAWILDCORE_API const TArray<FName>& GetTierBSpeciesIds();

        /**
         * PCR-4/5: DEFINITION-DRIVEN opt-in binding — the mesh path derives
         * from the species id by convention ("SK_Echo_<Name>"), so the engine
         * import pass activates the skinned body WITHOUT any engine-side code
         * patch (the §20c hand-patch stays Tier-A-only). Until the GLBs are
         * imported, LoadSynchronous fails closed and the PMC body stays — the
         * same opt-in contract the bosses use. Paths are DERIVED (never a
         * literal /Game/ string), so validator check 8 is unaffected.
         */
        ASTRAWILDCORE_API FString BuildTierBMechPath(const FName& EchoId);
        ASTRAWILDCORE_API FString BuildTierBAnimPath(const FName& EchoId, bool bMoveClip);

        /** True when the species id belongs to the Tier-B library. */
        ASTRAWILDCORE_API bool IsTierBSpecies(const FName& EchoId);
}
