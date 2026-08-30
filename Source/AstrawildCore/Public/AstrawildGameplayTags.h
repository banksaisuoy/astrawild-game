#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// Native gameplay tags — taxonomy per Docs/ASTRAWILD_GAMEPLAY_TAGS.md (directive §36).
// Native tags avoid depending on .ini asset scanning and are refactor-safe in C++.

// --- State.Creature.* (AI states) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Idle);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Explore);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_SearchFood);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Eat);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Sleep);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Socialize);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Investigate);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Flee);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Alert);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Combat);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Protect);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Follow);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Work);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_ReturnHome);
// Audit M-12: enum states previously missing their tags (dead/injured had no representation).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Injured);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Dead);
// Batch 3 — Item B: staggered reaction state (heavy hits / Pulse shock).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Creature_Staggered);

// --- State.Player.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Player_Dead);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Player_Dodging);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Player_Blocking);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_State_Player_PlacingBuilding);

// --- Status.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Poisoned);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Burning);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Frozen);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Wet);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Soaked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Rested);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Hungry);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Thirsty);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Cold);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Overheated);
// Batch 3 — Item A: element-driven status effects (Frost→Chilled, Pulse→Shocked)
// and Item B stagger. Burning/Poisoned above already map to Ember/Flora.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Chilled);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Shocked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Staggered);

// --- Element.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_None);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Light);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Ash);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Flora);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Frost);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Pulse);
// Audit M-12: Ember species (Echo_Emberfang) previously had no matching element tag.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Element_Ember);

// --- Damage.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Damage_Physical);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Damage_Elemental);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Damage_Fall);

// --- Item.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Material);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Consumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Equipment);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Creature);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Building);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Item_Quest);

// --- Interaction.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Harvest);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Capture);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Craft);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Rest);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Talk);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Interaction_Scan);

// --- Biome.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Biome_DawnFields);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Biome_LuminousRainforest);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Biome_SaltPlains);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Biome_AzureSnowline);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Biome_VeldaraRuins);

// --- Weather.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Clear);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Cloudy);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Rain);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_HeavyRain);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Storm);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Fog);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Heat);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Weather_Cold);

// --- Event.* (event bus payload types) ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_ItemCollected);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_EchoCaptured);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_EchoDefeated);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_HostileDefeated);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_BuildingPlaced);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_TechUnlocked);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_LocationReached);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_QuestObjectiveCompleted);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_RecipeCrafted);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_EchoFed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_EchoObserved);
// Batch 7 — The Shattered Vale: zone transitions (server-published, zone id as TargetId).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_ZoneEntered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_ZoneLeft);
// Final production run: active scanner completed a creature (journal milestone path).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_EchoScanned);
// Final production run: robotics deployments (drone/robot spawned).
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_DroneDeployed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Event_RobotDeployed);

// --- Faction.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Faction_Wild);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Faction_Player);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Faction_Hostile);

// --- Gameplay.* ---
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Gameplay_Debug);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Gameplay_Cheat);
