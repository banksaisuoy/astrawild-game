#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildEchoDexSubsystem.h"
#include "AstrawildEchoDexScreenWidget.generated.h"

class UScrollBox;
class UEditableTextBox;
class UTextBlock;
class UButton;

/**
 * EDX-1 row widget — one species line in the index list. Built purely in
 * C++ (mirrors UAstrawildCraftingRowWidget): state tag ("???" / seen /
 * captured), name, element, home zone; gold text + warm row tint when the
 * row matches the active search text (the Nexus-mod "highlighting" parity).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDexRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** One-line builder (idempotent — safe to call once per refresh). */
    void BuildRow(const UAstrawildEchoDefinition* Def, EAstrawildDexDiscoveryState State, bool bHighlighted);

    /** Owning screen receives clicks; rows carry no logic of their own. */
    TWeakObjectPtr<UAstrawildEchoDexScreenWidget> OwnerScreen;

    FName SpeciesId;

private:
    UFUNCTION()
    void HandleClicked();

    UPROPERTY()
    TObjectPtr<UTextBlock> RowText;
};

/**
 * EDX-1 "EchoDex" screen — the searchable species index UI (I key).
 *
 * Parity targets (evidence-based):
 *  - Pal List Searchbar mod (Nexus #4020) for Palworld: "a searchbar …
 *    highlighting, sorting Pals or jumping to page containing the Pals" —
 *    this screen ships search + row highlight + jump-to-first-match.
 *  - Aniimo index facets: element / role / stage filters — mirrored here as
 *    element / zone / role / discovery cycle-buttons (one tap each).
 *  - Paldeck discovery gating: Unknown rows read "???" and the detail panel
 *    stays locked until the species has been met; the Journal screen's
 *    BuildSpeciesDetailText supplies the knowledge-gated detail body.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildEchoDexScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Final-audit F-05 parity: focusable so ESC closes the screen in UIOnly mode. */
    UAstrawildEchoDexScreenWidget();

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    /** Re-run the current query and rebuild rows + status + detail. */
    UFUNCTION(BlueprintCallable, Category = "ASTRAWILD|Dex|UI")
    void RefreshIndex();

private:
    friend class UAstrawildDexRowWidget;

    /** Build the native Slate tree (title / status / search+facet row / list / detail / close). */
    void BuildNativeUi();

    /** Resolve the Dex subsystem (null early or for non-game worlds). */
    UAstrawildEchoDexSubsystem* GetDex() const;

    /** Rebuild the facet-button labels from the active query. */
    void RefreshFacetLabels();

    /** Show the detail panel for one species (locked text when Unknown). */
    void SelectSpecies(FName SpeciesId);

    /** Cycle helpers — each rotates one facet and refreshes. */
    UFUNCTION()
    void HandleElementClicked();
    UFUNCTION()
    void HandleZoneClicked();
    UFUNCTION()
    void HandleRoleClicked();
    UFUNCTION()
    void HandleDiscoveryClicked();

    /** Search box typing -> live re-query (the Palworld gap). */
    UFUNCTION()
    void HandleSearchChanged(const FText& Text);

    UFUNCTION()
    void HandleCloseClicked();

    /** Dex subsystem event -> refresh while open. */
    UFUNCTION()
    void HandleDexUpdated(FName SpeciesId);

    UPROPERTY()
    TObjectPtr<UEditableTextBox> SearchBox;

    UPROPERTY()
    TObjectPtr<UButton> ElementButton;

    UPROPERTY()
    TObjectPtr<UButton> ZoneButton;

    UPROPERTY()
    TObjectPtr<UButton> RoleButton;

    UPROPERTY()
    TObjectPtr<UButton> DiscoveryButton;

    UPROPERTY()
    TObjectPtr<UScrollBox> ResultsList;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> DetailText;

    /** Active query — search text + the four cycle-button facets. */
    FAstrawildDexQuery ActiveQuery;

    /** Currently selected species for the detail panel. */
    FName SelectedSpeciesId;
};
