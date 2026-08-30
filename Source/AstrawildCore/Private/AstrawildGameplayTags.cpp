#include "AstrawildGameplayTags.h"

// --- State.Creature.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Idle, "State.Creature.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Explore, "State.Creature.Explore");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_SearchFood, "State.Creature.SearchFood");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Eat, "State.Creature.Eat");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Sleep, "State.Creature.Sleep");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Socialize, "State.Creature.Socialize");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Investigate, "State.Creature.Investigate");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Flee, "State.Creature.Flee");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Alert, "State.Creature.Alert");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Combat, "State.Creature.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Protect, "State.Creature.Protect");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Follow, "State.Creature.Follow");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Work, "State.Creature.Work");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_ReturnHome, "State.Creature.ReturnHome");
// Audit M-12: previously missing states.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Injured, "State.Creature.Injured");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Dead, "State.Creature.Dead");
// Batch 3 — Item B: staggered reaction state.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Creature_Staggered, "State.Creature.Staggered");

// --- State.Player.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Player_Dead, "State.Player.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Player_Dodging, "State.Player.Dodging");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Player_Blocking, "State.Player.Blocking");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_State_Player_PlacingBuilding, "State.Player.PlacingBuilding");

// --- Status.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Poisoned, "Status.Poisoned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Burning, "Status.Burning");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Frozen, "Status.Frozen");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Wet, "Status.Wet");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Soaked, "Status.Soaked");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Rested, "Status.Rested");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Hungry, "Status.Hungry");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Thirsty, "Status.Thirsty");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Cold, "Status.Cold");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Overheated, "Status.Overheated");
// Batch 3 — Item A: element-driven statuses + Item B stagger.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Chilled, "Status.Chilled");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Shocked, "Status.Shocked");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Staggered, "Status.Staggered");

// --- Element.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_None, "Element.None");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Light, "Element.Light");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Ash, "Element.Ash");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Flora, "Element.Flora");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Frost, "Element.Frost");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Pulse, "Element.Pulse");
// Audit M-12: Ember was missing — Echo_Emberfang had no tag representation.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Element_Ember, "Element.Ember");

// --- Damage.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Damage_Physical, "Damage.Physical");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Damage_Elemental, "Damage.Elemental");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Damage_Fall, "Damage.Fall");

// --- Item.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Material, "Item.Material");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Consumable, "Item.Consumable");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Equipment, "Item.Equipment");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Creature, "Item.Creature");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Building, "Item.Building");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Item_Quest, "Item.Quest");

// --- Interaction.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Harvest, "Interaction.Harvest");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Capture, "Interaction.Capture");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Craft, "Interaction.Craft");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Rest, "Interaction.Rest");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Talk, "Interaction.Talk");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Interaction_Scan, "Interaction.Scan");

// --- Biome.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Biome_DawnFields, "Biome.DawnFields");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Biome_LuminousRainforest, "Biome.LuminousRainforest");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Biome_SaltPlains, "Biome.SaltPlains");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Biome_AzureSnowline, "Biome.AzureSnowline");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Biome_VeldaraRuins, "Biome.VeldaraRuins");

// --- Weather.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Clear, "Weather.Clear");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Cloudy, "Weather.Cloudy");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Rain, "Weather.Rain");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_HeavyRain, "Weather.HeavyRain");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Storm, "Weather.Storm");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Fog, "Weather.Fog");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Heat, "Weather.Heat");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Weather_Cold, "Weather.Cold");

// --- Event.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_ItemCollected, "Event.ItemCollected");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_EchoCaptured, "Event.EchoCaptured");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_EchoDefeated, "Event.EchoDefeated");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_HostileDefeated, "Event.HostileDefeated");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_BuildingPlaced, "Event.BuildingPlaced");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_TechUnlocked, "Event.TechUnlocked");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_LocationReached, "Event.LocationReached");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_QuestObjectiveCompleted, "Event.QuestObjectiveCompleted");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_RecipeCrafted, "Event.RecipeCrafted");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_EchoFed, "Event.EchoFed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_EchoObserved, "Event.EchoObserved");
// Batch 7 — The Shattered Vale: zone transitions.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_ZoneEntered, "Event.ZoneEntered");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_ZoneLeft, "Event.ZoneLeft");
// Final production run: active scanner + robotics deployments.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_EchoScanned, "Event.EchoScanned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_DroneDeployed, "Event.DroneDeployed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_RobotDeployed, "Event.RobotDeployed");
// Production V2: world dynamism.
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_WeatherChanged, "Event.WeatherChanged");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_WorldEventStarted, "Event.WorldEventStarted");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_WorldEventEnded, "Event.WorldEventEnded");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Event_PoiDiscovered, "Event.PoiDiscovered");

// --- Faction.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Faction_Wild, "Faction.Wild");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Faction_Player, "Faction.Player");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Faction_Hostile, "Faction.Hostile");

// --- Gameplay.* ---
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Gameplay_Debug, "Gameplay.Debug");
UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Gameplay_Cheat, "Gameplay.Cheat");
