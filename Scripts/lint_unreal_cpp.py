#!/usr/bin/env python3
"""
lint_unreal_cpp.py — LONG-RUN DIRECTIVE L4: pure-Python static C++ linter
==========================================================================
164 files / +33,343 lines have never been seen by a compiler. This linter
reduces that risk with REAL static analysis (no compilation, no engine):

  A. UCLASS/USTRUCT/UENUM macro integrity — every UCLASS() body contains
     GENERATED_BODY(); every UENUM() is an enum class with uint8 underlying
     type (UHT requirement); macro ordering (Uxxx() then type then body).
  B. .generated.h LAST include in every reflected header.
  C. UPROPERTY type resolution — every member type is defined in-file,
     included, forward-declared, or an engine/known type.
  D. UPROPERTY/UFUNCTION specifier validity — unknown specifier tokens
     and known-bad combinations.
  E. .cpp symbol closure — every AAstrawild*/UAstrawild*/FAstrawild* type
     used in each .cpp has a plausible provider (own header/include).
  F. header/impl drift — methods declared in .h but never defined in the
     paired .cpp and not inline/pure-virtual; definitions in .cpp missing
     from any header.
  G. module closure — engine headers included imply Build.cs modules.
  H. UE5 API drift patterns — only citable, well-documented patterns.
  I. log category discipline — DECLARE/DEFINE pairing and duplicates.

Exit 0 = no ERROR-severity issues (warnings allowed). Use --strict for
zero-tolerance.
"""

from __future__ import annotations

import os
import re
import sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "Source", "AstrawildCore")
PUB, PRIV = os.path.join(SRC, "Public"), os.path.join(SRC, "Private")

ERRORS = []
WARNINGS = []
STATS = defaultdict(int)


def err(code, rel, line, msg):
    ERRORS.append((code, rel, line, msg))


def warn(code, rel, line, msg):
    WARNINGS.append((code, rel, line, msg))


def relpath(path):
    return os.path.relpath(path, REPO).replace("\\", "/")


# ---------------------------------------------------------------------------
# Known engine/UE type vocabulary (available via CoreUObject/Core minimal
# includes or the PCH; never counted as unresolved)
# ---------------------------------------------------------------------------

ENGINE_TYPES = {
    "FVector", "FVector2D", "FVector4", "FRotator", "FTransform", "FQuat",
    "FString", "FName", "FText", "FLinearColor", "FColor", "FDateTime",
    "FGuid", "FBox2D", "FBox", "FSphere", "FCurve", "FRandomStream",
    "FTimerHandle", "FHitResult", "FTableRowBase", "FScriptDelegate",
    "FDelegateHandle", "FOnGPS", "FVector2f", "FIntPoint", "FIntVector",
    "TArray", "TSet", "TMap", "TSubclassOf", "TSoftObjectPtr",
    "TSoftClassPtr", "TEnumAsByte", "TOptional", "TPair", "TFunction",
    "uint8", "uint16", "uint32", "uint64", "int8", "int16", "int32",
    "int64", "float", "double", "bool", "void", "auto", "SIZE_T",
    "FDataTableRowHandle", "FGameplayTag", "FGameplayTagContainer",
    "FInputActionValue", "FInputChord", "FKey", "FAnalogInputEvent",
    "EMouseCursor", "EAppReturnType", "ESlateVisibility", "ECheckBoxState",
    "EVisibility", "EInputEvent", "ETextCommit", "EAudioFader",
    "ENetRole", "ENetDormancy", "EComponentCreationType", "EMobility",
    "TWeakObjectPtr", "TWeakPtr", "TSharedPtr", "TSharedRef",
    "FThreadSafeCounter", "FCriticalSection", "FScopeLock",
    "FPlatformTime", "FMath", "FArgument", "FOutputDevice", "FMsg",
    "FPlatformProcess", "FPaths", "FCommandLine", "FGenericPlatform",
    "UStruct", "UClass", "UObject", "UInterface", "UPackage", "UField",
    "UEnum", "UFunction", "UPackage", "UNumericProperty", "UProperty",
    "UActorComponent", "USceneComponent", "UPrimitiveComponent",
    "UStaticMeshComponent", "USkeletalMeshComponent", "UCapsuleComponent",
    "USphereComponent", "UBoxComponent", "UShapeComponent",
    "UInputComponent", "UEnhancedInputComponent", "UInputMappingContext",
    "UInputAction", "UMovementComponent", "UCharacterMovementComponent",
    "UFloatingPawnMovement", "USpringArmComponent", "UCameraComponent",
    "USkyLightComponent", "UDirectionalLightComponent",
    "UExponentialHeightFogComponent", "UAudioComponent",
    "UParticleSystemComponent", "UNiagaraComponent", "UNiagaraSystem",
    "UNiagaraFunctionLibrary", "UMaterialInstanceDynamic", "UMaterialInterface",
    "UMaterial", "UTexture2D", "USoundBase", "USoundWave", "USoundClass",
    "USkeletalMesh", "UStaticMesh", "UAnimInstance", "UAnimMontage",
    "UAnimSequenceBase", "UAnimBlueprint", "UDataTable", "UFont",
    "UUserWidget", "UBlueprint", "UBlueprintGeneratedClass",
    "UWidget", "UWidgetTree", "UCanvasPanel", "UVerticalBox", "UHorizontalBox",
    "UButton", "UTextBlock", "UProgressBar", "UImage", "UScrollBox",
    "USizeBox", "UOverlay", "UBorder", "UEditableTextBox", "UCheckBox",
    "USlider", "UComboBoxString", "URichTextBlock", "USpacer", "UGridPanel",
    "UCanvasPanelSlot", "UBorderSlot", "UOverlaySlot",
    "AAActor", "AActor", "APawn", "ACharacter", "AController",
    "APlayerController", "AAIController", "AGameModeBase", "AGameMode",
    "AGameStateBase", "AGameState", "APlayerState", "AHUD", "AInfo",
    "AWorldSettings", "AStaticMeshActor", "ADirectionalLight", "ASkyLight",
    "ASkyAtmosphere", "AExponentialHeightFog", "APlayerStart", "AVolume",
    "ASpawnPoint", "ADefaultPawn", "ASpectatorPawn", "ACharacter",
    "IInputDevice", "IEntitySystem", "UGameInstance", "UGameInstanceSubsystem",
    "UWorld", "ULevel", "ULevelStreaming", "UEngine", "UGameViewportClient",
    "ULocalPlayer", "UNetDriver", "UPendingNetGame", "UEngineLoop",
    "FTimerManager", "FWorldDelegates", "FCoreDelegates", "FApp",
    "UGameplayStatics", "UKismetSystemLibrary", "UKismetMathLibrary",
    "UBlueprintEditorLibrary", "UEditorLevelLibrary", "UEditorAssetLibrary",
    "UAssetTools", "UAssetToolsHelpers", "UActorFactories",
    "FAssetData", "FAssetRegistryModule", "IAssetRegistry",
    "FEditorDelegates", "FLevelEditorModule", "UMaterialEditingLibrary",
    "UFlexibleScaler", "SGraphNode", "SWidget", "SCompoundWidget",
    "FReply", "FGeometry", "FKeyEvent", "FPointerEvent", "FModifierKeyState",
    "FSlateBrush", "FSlateColor", "FSlateFontInfo", "FMargin", "FVector2D",
    "ECheckBoxState", "ETextJustify", "EHorizontalAlignment",
    "EVerticalAlignment", "EOrientation", "ESelectable", "EIsActive",
    "UDataTableFunctionLibrary", "UCurveTable", "UCurveFloat",
    "UDeveloperSettings", "UGameplayTagsSettings", "UGameplayTagAssetInterface",
    "ITargetPlatform", "FNamePool", "ELifetimeCondition", "FFastArraySerializer",
    "FRepMovement", "FActorSpawnParameters", "FActorInitializationState",
    "FActorDestroyReason", "FOverlapResult", "EActorUpdateOverlapsMethod",
    "FNavAgentProperties", "FNavAgentConfig", "UNavigationSystemV1",
    "EPathFollowingResult", "UPathFollowingComponent", "FAIDescription",
    "IAbilityTrigger", "EBootstrapStats", "FScopedSlowStack",
    "FScopedProfileStack", "UE_LOG", "check", "checkf", "ensure",
    "ensureMsgf", "verify", "UE_EDITOR", "WITH_EDITOR", "PLATFORM_WINDOWS",
}

ENGINE_ENUM_VALUES = {
    "EAstrawildZone", "EAstrawildElementType",  # project enums handled below
}

# UPROPERTY / UFUNCTION specifier vocabulary (UE 5.x)
UPROPERTY_SPECIFIERS = {
    "EditAnywhere", "EditDefaultsOnly", "EditInstanceOnly", "EditFixedSize",
    "EditReadOnly", "BlueprintReadOnly", "BlueprintReadWrite",
    "BlueprintCallable", "BlueprintSetter", "BlueprintGetter",
    "Category", "meta", "Config", "GlobalUserConfig", "Localized",
    "SaveGame", "Transient", "Replicated", "ReplicatedUsing",
    "SimpleDisplay", "AdvancedDisplay", "DisplayPriority", "AssetRegistry",
    "VisibleAnywhere", "VisibleDefaultsOnly", "VisibleInstanceOnly",
    "BlueprintAssignable", "BlueprintSpawnableComponent",
    "ChildPropertyName", "CustomType", "DefaultValue", "Deprecated",
    "DisplayName", "FullyExpand", "Instanced", "Interp", "InternalOnlyMeta",
    "keywords", "MacroDollar", "NoAutoRegister", "NoClear", "NoEditInline",
    "NonPIEDuplicateTransient", "NonTransactional", "NotReplicated",
    "ReplicationCallback", "ReturnDisplayName", "ScriptCallable",
    "ScriptReadable", "ShortTooltip", "ShowOnlyInnerProperties",
    "SkipSerialization", "Standalone", "Untracked", "UPARAM", "WorldContext",
}
UFUNCTION_SPECIFIERS = {
    "BlueprintImplementableEvent", "BlueprintNativeEvent",
    "BlueprintPure", "BlueprintCallable", "BlueprintCosmetic",
    "Category", "meta", "BlueprintInternalUseOnly",
    "BlueprintInternalUseOnlyMeta", "CallInEditor", "Client", "Server",
    "ServiceRequest", "ServiceResponse", "Reliable", "Unreliable",
    "WithValidation", "SealedEvent", "Exec", "Native", "NetMulticast",
    "CustomThunk", "BlueprintAuthorityOnly", "BlueprintWorldContext",
    "NotBlueprintCallable", "DeprecatedFunction", "DeprecationWarning",
    "CommutativeAssociativeBinaryOperator", "AdvancedDisplay",
    "AdvancedDisplay", "ReturnDisplayName", "DynamicOutputParam",
    "AutoCreateRefTerm", "BlueprintThreadSafe", "Latent",
    "LatentInfo", "ExpandEnumAsExecs", "ExpandBoolAsExecs", "Varargs",
    "UMETA", "DevelopmentOnly", "CustomStructureParam", "DefaultToSelf",
    "HidePin", "InternalUseParam", "ArrayParm", "ArrayTypeDependentParams",
    "BlueprintSetter", "BlueprintGetter", "CPP", "Untracked",
}

# known-bad specifier combinations
BAD_COMBOS = [
    ("UPROPERTY", {"EditAnywhere", "VisibleDefaultsOnly"},
     "Edit* and Visible* are mutually exclusive access specifiers"),
    ("UPROPERTY", {"EditDefaultsOnly", "VisibleAnywhere"},
     "Edit* and Visible* are mutually exclusive access specifiers"),
    ("UPROPERTY", {"EditInstanceOnly", "VisibleInstanceOnly"},
     "Edit* and Visible* are mutually exclusive access specifiers"),
    ("UPROPERTY", {"BlueprintReadOnly", "BlueprintReadWrite"},
     "BlueprintReadOnly and BlueprintReadWrite are mutually exclusive"),
    ("UFUNCTION", {"BlueprintPure", "BlueprintCallable"},
     "BlueprintPure implies BlueprintCallable — both is legal but "
     "redundant (warning only)"),
    ("UFUNCTION", {"Client", "Server", "NetMulticast"},
     "Client/Server/NetMulticast are mutually exclusive net modes"),
]

# ---------------------------------------------------------------------------
# file inventory
# ---------------------------------------------------------------------------


def load(path):
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        return fh.read().split("\n")


headers = sorted(
    os.path.join(PUB, f) for f in os.listdir(PUB) if f.endswith(".h"))
cpps = sorted(
    os.path.join(PRIV, f) for f in os.listdir(PRIV) if f.endswith(".cpp"))


# ---------------------------------------------------------------------------
# symbol tables
# ---------------------------------------------------------------------------

defined_types = {}        # type name -> header relpath
includes_by_file = {}     # header rel -> [include basenames]
fwd_decls_by_file = {}    # header rel -> [forward-declared names]
struct_classes = {}       # header rel -> [(kind, name, brace_depth_line)]


def scan_headers():
    for path in headers:
        rel = relpath(path)
        lines = load(path)
        includes = []
        fwd = []
        depth = 0
        for i, ln in enumerate(lines, start=1):
            inc = re.match(r'^\s*#include\s+[<"]([^">]+)[">]', ln)
            if inc:
                includes.append(os.path.basename(inc.group(1)))
            # forward declarations: class X; / struct X; / class UCLASSNAME;
            fd = re.match(r"^\s*(?:class|struct)\s+([AUF]\w+)\s*;\s*(?://.*)?$", ln)
            if fd and depth == 0:
                fwd.append(fd.group(1))
            for ch in ln:
                if ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
            # type definitions at class/struct/enum scope
            cd = re.match(r"^\s*(?:class|struct)\s+(?:ASTRAWILDCORE_API\s+)?([AUF]\w+)\s*(?::|{|$)", ln)
            if cd:
                defined_types.setdefault(cd.group(1), rel)
            ed = re.match(r"^\s*enum\s+class\s+(?:ASTRAWILDCORE_API\s+)?([EF]\w+)", ln)
            if ed:
                defined_types.setdefault(ed.group(1), rel)
            # delegate types (DECLARE_DYNAMIC_MULTICAST_DELEGATE_X(FName, ...)
            # / DECLARE_MULTICAST_DELEGATE / DECLARE_DYNAMIC_DELEGATE ...)
            dd = re.match(
                r"^\s*DECLARE_(?:DYNAMIC_)?(?:MULTICAST_)?DELEGATE[\w]*\(\s*"
                r"([FA]\w+)", ln)
            if dd:
                defined_types.setdefault(dd.group(1), rel)
        includes_by_file[rel] = includes
        fwd_decls_by_file[rel] = fwd
    # definitions in .cpp local helper namespaces count as defined too
    for path in cpps:
        rel = relpath(path)
        for ln in load(path):
            cd = re.match(r"^\s*(?:class|struct)\s+([AUF]\w+)\s*(?::|{|$)", ln)
            if cd:
                defined_types.setdefault(cd.group(1), rel)
            ed = re.match(r"^\s*enum\s+(?:class\s+)?([EF]\w+)", ln)
            if ed:
                defined_types.setdefault(ed.group(1), rel)
            # automation-test classes are defined by the IMPLEMENT macros
            at = re.match(
                r"^\s*IMPLEMENT_(?:SIMPLE|CUSTOM)_AUTOMATION_TEST\("
                r"\s*(\w+)", ln)
            if at:
                defined_types.setdefault(at.group(1), rel)

    # transitive include closure per header (project headers only)
    header_rels = [relpath(h) for h in headers]
    global basename_to_rel
    basename_to_rel = {os.path.basename(r): r for r in header_rels}

    def closure_of(rel):
        seen = set()
        stack = list(includes_by_file.get(rel, []))
        while stack:
            base = stack.pop()
            if base in seen:
                continue
            seen.add(base)
            dep = basename_to_rel.get(base)
            if dep:
                stack.extend(includes_by_file.get(dep, []))
        return seen

    global include_closure
    include_closure = {rel: closure_of(rel) for rel in header_rels}


# ---------------------------------------------------------------------------
# A. macro integrity
# ---------------------------------------------------------------------------

def check_macros():
    for path in headers:
        rel = relpath(path)
        lines = load(path)
        # every UCLASS( ... ) is followed by class X ... { GENERATED_BODY();
        for i, ln in enumerate(lines):
            if re.match(r"^\s*UCLASS\s*\(", ln):
                # find the opening brace of the class body
                j = i
                depth = 0
                found_open = False
                while j < len(lines) and j < i + 20:
                    for ch in lines[j]:
                        if ch == "{":
                            found_open = True
                        if found_open:
                            break
                    if found_open:
                        break
                    j += 1
                if not found_open:
                    err("A/UCLASS-BRACE", rel, i + 1,
                        "UCLASS() not followed by a class body within 20 lines")
                    continue
                # GENERATED_BODY() must appear within the first lines of body
                window = "\n".join(lines[j:j + 6])
                if "GENERATED_BODY()" not in window:
                    err("A/UCLASS-GENERATED", rel, i + 1,
                        "UCLASS() body lacks GENERATED_BODY() near the top")
                STATS["UCLASS"] += 1
            if re.match(r"^\s*USTRUCT\s*\(", ln):
                j = i
                found_open = False
                while j < len(lines) and j < i + 20:
                    if "{" in lines[j]:
                        found_open = True
                        break
                    j += 1
                if not found_open:
                    err("A/USTRUCT-BRACE", rel, i + 1,
                        "USTRUCT() not followed by a struct body")
                    continue
                window = "\n".join(lines[j:j + 4])
                if "GENERATED_BODY()" not in window and \
                        "GENERATED_USTRUCT_BODY()" not in window:
                    err("A/USTRUCT-GENERATED", rel, i + 1,
                        "USTRUCT() body lacks GENERATED_BODY()")
                STATS["USTRUCT"] += 1
            if re.match(r"^\s*UENUM\s*\(", ln):
                # next non-meta line should be enum class X : uint8
                window = "\n".join(lines[i:i + 3])
                if not re.search(r"enum\s+class\s+\w+\s*:\s*uint8", window):
                    warn("A/UENUM-UINT8", rel, i + 1,
                         "UENUM() is not `enum class X : uint8` "
                         "(UHT requires uint8 underlying type)")
                STATS["UENUM"] += 1


# ---------------------------------------------------------------------------
# B. .generated.h last include
# ---------------------------------------------------------------------------

def check_generated_include():
    for path in headers:
        rel = relpath(path)
        lines = load(path)
        has_reflection = any(re.match(r"^\s*U(CLASS|STRUCT|ENUM|INTERFACE)\s*\(", ln)
                             for ln in lines)
        if not has_reflection:
            continue
        gen = [i for i, ln in enumerate(lines)
               if re.match(r'^\s*#include\s+"[^"]+\.generated\.h"\s*$', ln)]
        if not gen:
            err("B/NO-GENERATED", rel, 0,
                "reflected header has no .generated.h include")
            continue
        last_include = max(i for i, ln in enumerate(lines)
                           if ln.strip().startswith("#include"))
        if gen[0] != last_include:
            err("B/GENERATED-NOT-LAST", rel, gen[0] + 1,
                ".generated.h is not the LAST #include "
                "(must be last for UHT; found later include at line {})"
                .format(last_include + 1))
        STATS["reflected_headers"] += 1


# ---------------------------------------------------------------------------
# C. UPROPERTY type resolution
# ---------------------------------------------------------------------------

TYPE_RE = re.compile(
    r"^\s*(?:mutable\s+)?((?:T(?:Array|Map|Set|SubclassOf|SoftObjectPtr|"
    r"SoftClassPtr|EnumAsByte|WeakObjectPtr|Optional|Pair)<[^>]+>|"
    r"[AUF]\w+)\s*\**)\s*(\w+)\s*(?:=[^;]*)?;")


def extract_type_names(type_token):
    """Type names inside a member type token. The word-boundary anchor
    means container fragments (Array inside TArray) never match; the
    letter class includes E for enums. EnumAsByte is a container token."""
    names = re.findall(r"\b[AUEF]\w+", type_token)
    return [n for n in names if n not in ("EnumAsByte",)]


def check_uproperty_types():
    for path in headers:
        rel = relpath(path)
        lines = load(path)
        own_types = {t for t, r in defined_types.items() if r == rel}
        included = set()
        for inc in includes_by_file.get(rel, []):
            included.add(inc)
            # the include's own defined types become visible
        included_types = set()
        for inc in includes_by_file.get(rel, []):
            # map include basename -> defining header rel
            for t, r in defined_types.items():
                if os.path.basename(r) == inc:
                    included_types.add(t)
        visible = (own_types | included_types
                   | set(fwd_decls_by_file.get(rel, ()))
                   | ENGINE_TYPES)
        pending = False
        for i, ln in enumerate(lines, start=1):
            if re.match(r"^\s*UPROPERTY\s*\(", ln):
                pending = True
                continue
            if pending:
                m = TYPE_RE.match(ln)
                if m:
                    for t in extract_type_names(m.group(1)):
                        if t not in visible:
                            warn("C/UPROPERTY-TYPE", rel, i + 0,
                                 "member type {!r} not visible via "
                                 "define/include/forward-decl/known-engine "
                                 "types".format(t))
                    pending = False
                elif ln.strip() and not ln.strip().startswith("//"):
                    pending = False


# ---------------------------------------------------------------------------
# D. specifier validity
# ---------------------------------------------------------------------------

def _split_specifiers(body):
    """Paren-aware top-level comma split so meta=(ClampMin=..., ClampMax=...)
    stays one token."""
    parts, buf, depth = [], [], 0
    in_str = False
    for ch in body:
        if in_str:
            buf.append(ch)
            if ch == '"':
                in_str = False
        elif ch == '"':
            in_str = True
            buf.append(ch)
        elif ch in '({':
            depth += 1
            buf.append(ch)
        elif ch in ')}':
            depth -= 1
            buf.append(ch)
        elif ch == ',' and depth == 0:
            parts.append(''.join(buf).strip())
            buf = []
        else:
            buf.append(ch)
    tail = ''.join(buf).strip()
    if tail:
        parts.append(tail)
    return parts


def check_specifiers():
    for path in headers + cpps:
        rel = relpath(path)
        lines = load(path)
        for i, ln in enumerate(lines, start=1):
            for macro, vocab, kind in (
                    ("UPROPERTY", UPROPERTY_SPECIFIERS, "UPROPERTY"),
                    ("UFUNCTION", UFUNCTION_SPECIFIERS, "UFUNCTION")):
                if re.match(r"^\s*" + macro + r"\s*\(", ln):
                    # specifiers may span lines — collapse until ')'
                    j, depth = i, ln.count("(") - ln.count(")")
                    blob = ln
                    while depth > 0 and j < len(lines):
                        j += 1
                        blob += " " + lines[j - 1].strip()
                        depth += lines[j - 1].count("(") - lines[j - 1].count(")")
                    body = blob[blob.find("(") + 1:]
                    body = body[:body.rfind(")")]
                    toks = _split_specifiers(body)
                    names = set()
                    for t in toks:
                        name = t.split("=")[0].strip()
                        if name:
                            names.add(name)
                    for name in names:
                        base = name.split("(")[0].strip()
                        if base and base not in vocab and \
                                not base.startswith('"') and \
                                base not in ("DisplayName",):
                            warn("D/SPECIFIER", rel, i,
                                 "{} unknown specifier {!r}".format(kind, base))
                    for kind2, combo, why in BAD_COMBOS:
                        if kind2 == kind and combo <= names:
                            if "redundant" in why:
                                warn("D/BAD-COMBO", rel, i,
                                     "{}: {}".format(kind, why))
                            else:
                                err("D/BAD-COMBO", rel, i,
                                    "{}: {}".format(kind, why))


# ---------------------------------------------------------------------------
# E. .cpp closure
# ---------------------------------------------------------------------------

def check_cpp_closure():
    for path in cpps:
        rel = relpath(path)
        lines = load(path)
        own_header = os.path.join(PUB, os.path.basename(path).replace(".cpp", ".h"))
        own_types = set()
        header_includes = []
        if os.path.exists(own_header):
            own_rel = relpath(own_header)
            own_types = {t for t, r in defined_types.items() if r == own_rel}
            # the paired header's includes are transitively visible (the cpp
            # always includes its own header first)
            header_includes = includes_by_file.get(own_rel, [])
        included_types = set()
        # types defined in the .cpp itself (automation test classes etc.)
        for t, r in defined_types.items():
            if r == rel:
                included_types.add(t)
        # transitive closure: every direct include's full closure, plus
        # the paired header's (the cpp always includes it first)
        reachable_bases = set()
        for ln in lines:
            inc = re.match(r'^\s*#include\s+[<"]([^">]+)[">]', ln)
            if inc:
                base = os.path.basename(inc.group(1))
                reachable_bases.add(base)
                dep = basename_to_rel.get(base)
                if dep:
                    reachable_bases |= include_closure.get(dep, set())
        if os.path.exists(own_header):
            own_rel = relpath(own_header)
            reachable_bases |= include_closure.get(own_rel, set())
            reachable_bases.update(includes_by_file.get(own_rel, []))
        for base in reachable_bases:
            for t, r in defined_types.items():
                if os.path.basename(r) == base:
                    included_types.add(t)
        # scan for AAstrawild*/UAstrawild*/FAstrawild* usages (comments
        # stripped: block-comment state tracked across lines, // to EOL)
        in_comment = False
        for i, ln in enumerate(lines, start=1):
            working = ln
            if in_comment:
                if "*/" in working:
                    working = working.split("*/", 1)[1]
                    in_comment = False
                else:
                    continue
            if "/*" in working:
                working = working.partition("/*")[0]
                in_comment = True
            if "//" in working:
                working = working.split("//", 1)[0]
            for m in re.finditer(r"\b([AUF]Astrawild\w+)\b", working):
                t = m.group(1)
                if t not in own_types and t not in included_types:
                    # allow engine-side things like the module API macro
                    warn("E/CPP-CLOSURE", rel, i,
                         "type {!r} used without direct include "
                         "(may resolve transitively)".format(t))


# ---------------------------------------------------------------------------
# F. header/impl drift
# ---------------------------------------------------------------------------

def check_signature_drift():
    for path in headers:
        rel = relpath(path)
        stem = os.path.basename(path)[:-2]
        cpp = os.path.join(PRIV, stem + ".cpp")
        if not os.path.exists(cpp):
            continue
        h_lines = load(path)
        c_text = "\n".join(load(cpp))
        h_text = "\n".join(h_lines)
        # real class names declared in this header (U/A prefix intact)
        class_names = re.findall(
            r"^\s*class\s+(?:ASTRAWILDCORE_API\s+)?([UA]\w+)", h_text, re.M)
        if not class_names:
            continue
        decls = []
        in_block_comment = False
        prev_macro = ""
        for i, ln in enumerate(h_lines, start=1):
            stripped = ln.strip()
            # track /* */ comment state so comment prose never parses as decls
            working = ln
            if in_block_comment:
                if "*/" in working:
                    working = working.split("*/", 1)[1]
                    in_block_comment = False
                else:
                    prev_macro = ""
                    continue
            if "/*" in working:
                before, _sep, _rest = working.partition("/*")
                working = before
                in_block_comment = True
            if not working.strip() or working.strip().startswith(("*", "//")):
                if re.match(r"\s*UFUNCTION", working):
                    prev_macro = working
                prev_macro = "" if not re.match(r"\s*UFUNCTION", working) else prev_macro
                continue
            if re.match(r"\s*UFUNCTION", working):
                prev_macro = working
                continue
            m = re.match(
                r"^\s*(?:virtual\s+)?(?:[\w:<>,\s&\*]+?)\s+(\w+)\s*\("
                r"[^;]*\)\s*(?:const)?\s*(?:override|final)?\s*;\s*$", working)
            if m and "operator" not in working and not stripped.startswith("//"):
                is_bp_event = "BlueprintImplementableEvent" in prev_macro or \
                    "BlueprintNativeEvent" in prev_macro
                decls.append((m.group(1), i, ln.strip(), is_bp_event))
            if not re.match(r"^\s*U[A-Z]", working):
                prev_macro = ""
        for name, line, raw, is_bp_event in decls:
            if is_bp_event:
                continue    # BlueprintImplementableEvent: no C++ body by design
            if "= 0;" in raw or "virtual" in raw:
                continue
            if name.startswith(("DECLARE_", "GET_", "UFUNCTION")):
                continue
            if name.upper() == name:        # macro invocations, not methods
                continue
            inline_def = re.search(
                r"\b" + re.escape(name) + r"\s*\([^;]*\)\s*(?:const\s*)?\{",
                h_text)
            # accept any qualifier (class, struct, or namespace) and the RPC
            # _Implementation / _Validate suffixes UHT generates
            candidates = [name, name + "_Implementation", name + "_Validate"]
            defined = any(
                re.search(r"\b\w+::" + re.escape(cand) + r"\s*\(", c_text)
                for cand in candidates)
            if not defined and not inline_def:
                warn("F/H-IMPL-DRIFT", rel, line,
                     "method {}() declared but no definition found in "
                     "paired .cpp or inline body".format(name))


# ---------------------------------------------------------------------------
# G. module closure
# ---------------------------------------------------------------------------

INCLUDE_TO_MODULE = {
    "NiagaraSystem.h": "Niagara", "NiagaraComponent.h": "Niagara",
    "NiagaraFunctionLibrary.h": "Niagara",
    "EnhancedInputComponent.h": "EnhancedInput",
    "EnhancedInputSubsystems.h": "EnhancedInput",
    "InputMappingContext.h": "EnhancedInput",
    "InputAction.h": "EnhancedInput",
    "ProceduralMeshComponent.h": "ProceduralMeshComponent",
    "NavigationSystem.h": "NavigationSystem",
    "AIController.h": "AIModule",
    "BrainComponent.h": "AIModule",
    "BehaviorTree": "AIModule",
    "UserWidget.h": "UMG", "Widget.h": "UMG", "Button.h": "UMG",
    "TextBlock.h": "UMG", "CanvasPanel.h": "UMG", "ProgressBar.h": "UMG",
    "Image.h": "UMG", "ScrollBox.h": "UMG", "VerticalBox.h": "UMG",
    "HorizontalBox.h": "UMG", "SizeBox.h": "UMG", "Overlay.h": "UMG",
    "Border.h": "UMG", "EditableTextBox.h": "UMG", "CheckBox.h": "UMG",
    "ComboBoxString.h": "UMG", "GameplayTags.h": "GameplayTags",
    "GameplayTagContainer.h": "GameplayTags",
    "Sockets.h": "Sockets", "SocketSubsystem.h": "Sockets",
}


def check_modules():
    build_cs = os.path.join(SRC, "AstrawildCore.Build.cs")
    text = open(build_cs, encoding="utf-8-sig", errors="replace").read()
    mods = set(re.findall(r'"(\w+)"', text))
    used = set()
    for path in headers + cpps:
        for ln in load(path):
            inc = re.match(r'^\s*#include\s+[<"]([^">]+)[">]', ln)
            if inc:
                base = os.path.basename(inc.group(1))
                for key, mod in INCLUDE_TO_MODULE.items():
                    if base == key:
                        used.add(mod)
    missing = used - mods
    if missing:
        err("G/MODULE", relpath(build_cs), 0,
            "modules used via includes but missing from Build.cs: {}"
            .format(sorted(missing)))
    STATS["modules_used"] = len(used)


# ---------------------------------------------------------------------------
# H. UE5 API drift patterns (citable only)
# ---------------------------------------------------------------------------

DRIFT_PATTERNS = [
    (re.compile(r"\bTBaseDelegate\b"),
     "TBaseDelegate was removed (UE4.12+): use TDelegate", "ERROR"),
    (re.compile(r"\bFPostConstructInitializeProperties\b"),
     "FPostConstructInitializeProperties removed (UE4.9): use FObjectInitializer", "ERROR"),
    (re.compile(r"FApp::GetGameTime\b"),
     "FApp::GetGameTime deprecated: use FPlatformTime::Seconds", "WARN"),
    (re.compile(r'#include\s+[<"]Runtime/'),
     "legacy Runtime/-prefixed include path (UE5 module include convention)", "WARN"),
    (re.compile(r"\bOnComponentHit\s*\("),
     "verify OnComponentHit signature (UE5: takes FHitResult ref)", "INFO"),
]


def check_drift():
    for path in headers + cpps:
        rel = relpath(path)
        for i, ln in enumerate(load(path), start=1):
            for pat, msg, sev in DRIFT_PATTERNS:
                if pat.search(ln):
                    if sev == "ERROR":
                        err("H/API-DRIFT", rel, i, msg)
                    else:
                        warn("H/API-DRIFT", rel, i, msg)


# ---------------------------------------------------------------------------
# I. log categories
# ---------------------------------------------------------------------------

def check_log_categories():
    declares = {}
    defines = defaultdict(list)
    for path in headers:
        rel = relpath(path)
        for i, ln in enumerate(load(path), start=1):
            m = re.match(r"^\s*DECLARE_LOG_CATEGORY_EXTERN\((\w+)", ln)
            if m:
                declares[m.group(1)] = (rel, i)
    for path in cpps:
        rel = relpath(path)
        for i, ln in enumerate(load(path), start=1):
            m = re.match(r"^\s*DEFINE_LOG_CATEGORY\((\w+)\)", ln)
            if m:
                defines[m.group(1)].append((rel, i))
            m2 = re.match(r"^\s*DECLARE_LOG_CATEGORY_EXTERN\((\w+)", ln)
            if m2:
                # DECLARE in a .cpp body (non-extern local category) is legal
                # only with DEFINE in the same file
                if m2.group(1) not in defines:
                    defines[m2.group(1)].append((rel, i))
    for cat, (rel, line) in declares.items():
        n = len(defines.get(cat, []))
        if n == 0:
            err("I/LOG-NO-DEFINE", rel, line,
                "DECLARE_LOG_CATEGORY_EXTERN({}) has no DEFINE_LOG_CATEGORY".format(cat))
        elif n > 1:
            err("I/LOG-DUP-DEFINE", rel, line,
                "category {} defined {} times: {}".format(
                    cat, n, defines[cat]))
    for cat in defines:
        if cat not in declares:
            warn("I/LOG-LOCAL", "", 0,
                 "log category {} defined without extern declare "
                 "(local category)".format(cat))
    STATS["log_categories"] = len(declares)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    print("[lint] scanning {} headers + {} cpps ...".format(
        len(headers), len(cpps)))
    scan_headers()
    check_macros()
    check_generated_include()
    check_uproperty_types()
    check_specifiers()
    check_cpp_closure()
    check_signature_drift()
    check_modules()
    check_drift()
    check_log_categories()

    print("[lint] stats: " + ", ".join(
        "{}={}".format(k, v) for k, v in sorted(STATS.items())))

    print("\n[lint] ERRORS ({}):".format(len(ERRORS)))
    for code, rel, line, msg in sorted(ERRORS)[:60]:
        print("  {:<24} {}:{}  {}".format(code, rel, line, msg))
    if len(ERRORS) > 60:
        print("  ... and {} more".format(len(ERRORS) - 60))
    print("\n[lint] WARNINGS ({}):".format(len(WARNINGS)))
    for code, rel, line, msg in sorted(WARNINGS)[:80]:
        print("  {:<24} {}:{}  {}".format(code, rel, line, msg))
    if len(WARNINGS) > 80:
        print("  ... and {} more".format(len(WARNINGS) - 80))

    print("\n[lint] SUMMARY: {} errors, {} warnings across {} files".format(
        len(ERRORS), len(WARNINGS), len(headers) + len(cpps)))
    strict = "--strict" in sys.argv
    if ERRORS or (strict and WARNINGS):
        print("[lint] *** LINT FAILED ***")
        return 1
    print("[lint] LINT PASSED (0 errors — warnings above are advisory)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
