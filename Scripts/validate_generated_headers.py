"""Check Unreal reflected headers for generated.h includes."""
from pathlib import Path

errors: list[str] = []
for path in (Path(__file__).resolve().parents[1] / "Source").rglob("*.h"):
    text = path.read_text(encoding="utf-8", errors="replace")
    if any(token in text for token in ("UCLASS(", "USTRUCT(", "UENUM(", "UINTERFACE(")) and "generated.h" not in text:
        errors.append(str(path))

if errors:
    print("Generated-header validation failed:")
    for error in errors:
        print(f"- {error}")
    raise SystemExit(1)

print("Generated-header validation passed.")
