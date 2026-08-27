# ASTRAWILD — Antigravity Reconciliation Protocol

## Why this exists

A handoff report may describe a completed Vertical Slice on an Antigravity workspace, but the repository is the only verifiable source of truth. The current GitHub repository has only `main`; the reported `release/vertical-slice-v1` branch and commit `f8cf5f1` are not present there at the time of audit.

## Required action on the Antigravity machine

From the workspace that contains the reported playable prototype:

```bash
git status
git branch --show-current
git log -1 --oneline
git remote -v
git add .
git commit -m "feat: import playable vertical slice from antigravity workspace"
git push -u origin release/vertical-slice-v1
```

If the branch does not exist locally, create it first:

```bash
git switch -c release/vertical-slice-v1
git add .
git commit -m "feat: import playable vertical slice from antigravity workspace"
git push -u origin release/vertical-slice-v1
```

Do not force-push or overwrite `main` until the tree and build status have been reviewed. If the current workspace has uncommitted generated Unreal files, include only files allowed by `.gitignore` and `.gitattributes`; do not commit `Binaries`, `Intermediate`, `Saved`, or `DerivedDataCache`.

## Evidence required with the push

Update `Docs/BUILD_STATUS.md` with:

- branch and commit hash;
- exact Unreal Engine and compiler version;
- compile target and result;
- map, Blueprint, Data Asset, animation and UI paths;
- test results and known issues;
- performance capture context and target hardware;
- asset license records in `Docs/ThirdPartyLicenses.md`.

Provide at least one screenshot or short video of the playable `L_Prototype` and one screenshot of the Content Browser showing the created `.umap` and `.uasset` files. The report must distinguish between a static code audit and an actual Unreal Compile/Playtest.

## Merge rule

Manus should review the branch tree before merging. If the Antigravity branch contains a more complete implementation, preserve the stronger systems but reconcile naming and contracts against the current `AstrawildCore` API. Resolve duplicate classes such as old/new Echo, Inventory, Crafting, Save and Capture systems instead of keeping two implementations with similar names.
