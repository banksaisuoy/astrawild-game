# P1 — Niagara and Sound Feedback Contract

## Runtime implementation

`UAstrawildFeedbackComponent` is now part of the Player and Echo actors. It binds to the existing `UAstrawildCombatComponent` delegates `OnDamageDealt`, `OnDamageReceived`, and `OnComboStep`, plus the existing `UAstrawildCaptureComponent` delegates for success, failure, and feedback. It does not own gameplay damage or capture rules; it only presents events.

## Required Niagara systems

| ID | Expected path | Event source | Assignment property | Suggested parameters |
|---|---|---|---|---|
| `NS_SolarSparks` | `Content/Astrawild/VFX/Combat/NS_SolarSparks` | Solar damage / Ignited status | `SolarSparksVFX` | `User.Damage`, `User.ComboStep`, `User.ElementStrength`, `User.ImpactNormal` |
| `NS_GeoDust` | `Content/Astrawild/VFX/Combat/NS_GeoDust` | Geo damage / Shielded status | `GeoDustVFX` | `User.Damage`, `User.ComboStep`, `User.ShieldStrength` |
| `NS_TorrentSplash` | `Content/Astrawild/VFX/Combat/NS_TorrentSplash` | Torrent damage / Drenched status | `TorrentSplashVFX` | `User.Damage`, `User.HealAmount`, `User.ImpactNormal` |
| `NS_CaptureSuccess` | `Content/Astrawild/VFX/Capture/NS_CaptureSuccess` | `OnCaptureSuccess` | `CaptureSuccessVFX` | `User.CapturePower`, `User.PartySlot` |
| `NS_CaptureFail` | `Content/Astrawild/VFX/Capture/NS_CaptureFail` | `OnCaptureFailed` | `CaptureFailVFX` | `User.ShakesCompleted` |

## Required sound assets

Use `USoundBase` references in the Feedback component. Sound Cue assets can be assigned directly to the properties even though the property type is the common `USoundBase` base class.

| ID | Expected path | Property |
|---|---|---|
| `SC_Damage` | `Content/Astrawild/Audio/Combat/SC_Damage` | `DamageSound` |
| `SC_CriticalDamage` | `Content/Astrawild/Audio/Combat/SC_CriticalDamage` | `CriticalDamageSound` |
| `SC_ComboStep` | `Content/Astrawild/Audio/Combat/SC_ComboStep` | `ComboSound` |
| `SC_CaptureSuccess` | `Content/Astrawild/Audio/Capture/SC_CaptureSuccess` | `CaptureSuccessSound` |
| `SC_CaptureFail` | `Content/Astrawild/Audio/Capture/SC_CaptureFail` | `CaptureFailSound` |

## Assignment and test gate

Create a Player Blueprint and three Echo Blueprints that inherit from the C++ classes. Assign the same named assets in the Feedback component defaults. In PIE, confirm that damage invokes the element-specific effect, critical damage chooses the critical sound, combo steps play combo feedback, and both capture outcomes trigger the correct capture effect/sound. Record the actual Content Browser paths and a short capture sequence video in `Docs/BUILD_STATUS.md`.

Do not embed audio or visual assets from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or any other existing game. Record external assets and their licenses in the project license register.
