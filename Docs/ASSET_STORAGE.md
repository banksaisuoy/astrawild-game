# ASTRAWILD Asset Storage Policy

## GitHub

GitHub stores source code, Unreal configuration, documentation, small reference assets, and binary files that are actively versioned through Git LFS. Generated folders such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` must not be committed.

## Google Drive

**Project folder:** https://drive.google.com/drive/folders/1hkCl5lYaDu8Cn_uikzdc5kbrAkWbLlon

Google Drive stores the complete pre-production archive, large source packages, concept art collections, raw audio, high-resolution meshes, video references, exported builds, and backup snapshots. Each archive should include a date and project version, for example `ASTRAWILD_Prototype_0.1.0_2026-08-27.zip`.

## Naming

Use `TYPE_NAME_VERSION_DATE` for archives. Keep source asset folders separated from review exports. Do not overwrite a previous archive; create a new version so the team can return to a known-good snapshot.

## Linking

After the Google Drive folder is created, place the shared folder URL in this file and in the repository README. The repository should remain usable when the Drive folder is unavailable; Drive is a backup and large-asset store, not a runtime dependency.

## License

Every external asset must include its license, source URL, author, and allowed usage in `Docs/ThirdPartyLicenses.md`. Do not add asset packs, music, fonts, or scans to the project until their redistribution rights are known.
