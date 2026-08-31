# CP-09 — QUEST / NPC CONTENT: Dialogue, Story Flags & Village Life

**Goal:** NPCs are characters, not toast dispensers. **The dialogue system is
SOURCE_IMPLEMENTED this batch (Batch 3):** data-driven conversation trees, gated
player choices, consequences routing through existing authority pipelines, persistent
story flags in save v4. This pack documents the shipped system + the content roadmap.

---

## 1. Shipped system (source-level, compile pending UE5 verification)

**Data:** `UAstrawildDialogueTreeDefinition` (DialogueId/EntryNodeId/Nodes) —
`FAstrawildDialogueLine` (SpeakerName override + Text), `FAstrawildDialogueNode`
(Lines[] + Choices[]), `FAstrawildDialogueChoice`:

- **Conditions (AND):** RequiredQuestActiveId / RequiredQuestCompletedId /
  RequiredFlagId / ForbiddenFlagId (NAME_None = ignored).
- **Consequences (fixed order):** StartQuestId → SetFlagId → GiveItemId(+Qty) →
  GiveResearchPoints; then GotoNodeId / bEndDialogue / bOpenShop.

**Runtime:** `UAstrawildDialogueComponent` (player controller — story flags persist
through save v4 `DialogueFlags`) + `UAstrawildDialogueWidget` (pure-C++ UMG screen).
NPC interact routes to the tree when `DialogueTreeId` resolves; legacy quest-toast +
shop paths stay as fallback for unbound NPCs.

**6 trees shipped (CODE_DEFAULT):**

| Tree | NPC | Shows off |
|---|---|---|
| `Dialogue_WardenMaren` | Warden Maren (Dawnstead) | Quest offer (First Light) + lore + **report-back** beat gated on quest completion, one-time research reward |
| `Dialogue_TraderTam` | Trader Tam | **bOpenShop vendor hand-off** + one-time Gloomfang tip (ForbiddenFlag) |
| `Dialogue_ElderRowan` | Elder Rowan | Quest offer (Wings over the Vale) + old-world lore |
| `Dialogue_SkiffWardenKael` | Kael (Driftwood) | Quest offer (Sunken Vault) + vault warning |
| `Dialogue_GuardSela` | Guard Captain Sela | Night-raid survival tips + one-time watch-advice research grant |
| `Dialogue_OldSaltPerry` | Old Salt Perry | Pure village color + **chained flags** (tide story → sea pearl gift) |

**Tests:** `ASTRAWILD.Dialogue.TreeContract / ChoiceConditions / Consequences` (+3 → 51 total).

## 2. Content roadmap (Antigravity / next batches)

1. **Remaining 6 NPCs get trees:** Herbalist Wren (shop+remedies lore), Blacksmith
   Borin (shop+weapon lore), Guard Bram (Gloomfang color), Farmer Jori (Echo
   husbandry tips → farming tutorial), Fisher Nima (shop+tide tables), Skiff crew.
2. **Quest-chain integration:** FirstLight chain beats 1–8 currently toast-only —
   migrate offers/completions into trees (StartQuestId + RequiredQuestCompletedId
   covers all patterns; no new systems).
3. **Voice pass (post-launch):** `A_Dlg_<NPC>_<NodeId>_<LineIdx>` cues; the widget
   plays them per line if present (binding: per-tree audio table, soft refs).
4. **Speaker portraits:** tree nodes may carry portrait textures (BP layer; optional).

## 3. Quest content state

12 quests live (chain: FirstLight → FirstEcho → Homeground → Spark → DawnGuard →
ShepherdsDawn → HollowUnderlight → ValeBeyond → WingsOverTheVale → SunkenVault;
production adds SignalsInTheStatic → VanguardProtocol). Objective types: Collect /
CraftRecipe / ResearchTech / CaptureEcho / DefeatEcho / SurviveTime / VisitZone /
ReachLocation / DiscoverPOI. New content lands as data; **no quest-code changes needed
for the pack** — writing trees is the remaining authoring work.

## 4. Village life (Batch 8 base + this pack's polish)

- 12 NPCs, 2 villages (Dawnstead 8, Driftwood 3+Maren retrofit), patrol AI
  (walk 190/run 430, night campfire gathering 21:00–06:00, guard aggro 35 m).
- CP-08 §7 role idles + CP-06 §3 village accents (campfire crackle, market chatter).
- Dialogue interaction pauses + faces the player (C++ live).

## Acceptance

- [ ] Talk [E] to Maren → conversation screen; Accept starts First Light (HUD quest
      banner as today); Leave closes; ESC closes; input returns to game-only.
- [ ] After completing First Light, the Report choice appears once, grants 15
      research points, then never again (save/reload included).
- [ ] Tam: "Browse wares" closes dialogue and opens the same shop screen.
- [ ] Perry: pearl gift only after hearing the tide story once (chained flags).
- [ ] Story flags survive save/load (v4 payload).
- [ ] Unbound NPCs (e.g. Guard Bram, pre-tree) keep the legacy interact behavior.
