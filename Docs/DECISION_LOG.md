# ASTRAWILD — DECISION LOG (LONG-RUN DIRECTIVE)

> Every autonomous decision made without asking the owner during the
> long-run directive, with its reasoning. Newest last.

| # | Phase | Decision | Reasoning | Impact |
|---|---|---|---|---|
| D-1 | L1 | Reported the NEW extractor (not the repo docs) as the wrong side of the 204-vs-229 discrepancy | Every independent recount (grep + the repo's own enforced census gate) re-proved the docs; the 3-day-old extractor had missed AstrawildProductionContent.cpp entirely | Fixed in L2; Design/README.md claims became true |
| D-2 | L2 | Bumped the design-data schema to `astrawild-design-data/2` (26 domains) instead of quietly editing v1 | The domain set changed shape; validators and any future consumer deserve an explicit version boundary | Validator updated in the same commit |
| D-3 | L2 | Evolution-target stats recorded as DERIVED (formula-traced note) rather than inventing literal values | R1: the registration loop computes them from base stats at runtime — there is no literal to trace | design_data.json carries a note per row |
| D-4 | L2/L3 | Kept `unreal.RowStruct` OUT of the mock and OUT of generate_datatables; set the row struct via `DataTableFactory.struct` | R2: the factory property is the documented path; RowStruct was an unverifiable guess — deleted rather than faked | Mock table reached 55 VERIFIED / 0 UNVERIFIED |
| D-5 | L4 | Fixed the two linter-proven compile blockers directly in Source (3-line rename + 1 include) | Both were unambiguous: the phantom `UAstrawildPlayerController` type cannot compile, and the missing include has exactly one correct form | Minimum diff, all validators re-run green |
| D-6 | L4 | Left the dead `BuildContentDefaults()` declaration in place, documented as O-1 | Zero callers → zero link risk; removing a public header member is owner territory per R6 minimum-diff | COMPILE_RISK §4 |
| D-7 | L5 | generate_datatables creates ONLY the 3 domains with existing reflected USTRUCT row shapes; the other 20 domains staged as JSON + TODO_ASK_OWNER | Authoring new C++ row structs without a consumer would be dead data — an owner decision, not an autonomous one | Design/DataTable/*.json (748 staged rows) |
| D-8 | L5 | Showcase map's cinematic rig uses fail-closed `safe_set_property` for PostProcessVolume fields | A 5.8 property rename must degrade the look, never kill the day-one run | build_showcase_map.py |
| D-9 | L6 | Classified UE interface headers and module/target boilerplate as FULL (not STUB) | Header-only interfaces and Build.cs boilerplate are the COMPLETE form for their purpose; "skeleton" would be a false claim | Inventory: 183 FULL / 17 PARTIAL / 0 STUB |
| D-10 | L7 | Cited NO new asset sources; every placeholder maps to already-staged CC0 assets or in-engine authored VFX | "Claim no license you cannot cite" — nothing new was needed | ART_ASSET_PLAN §5 |
