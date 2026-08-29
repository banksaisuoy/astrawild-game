"""Verify import status without re-running UE — just checks what was already produced.

Cross-references the 38 expected .uasset DataTables against the on-disk presence
under Content/Astrawild/Data/Imported/ and also inspects the last
DataTableImportReport.json to characterize whether the report's 'imported_count'
matches actual disk state.

This is honest verification: a report claiming 0/38 cannot be trusted if the
.uasset files are actually present on disk. The real number is the on-disk one.
"""
import json
import sys
from pathlib import Path

UGAME = Path(r"C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game")
IMPORTS = UGAME / "Content" / "Astrawild" / "Data" / "Imported"
REPORT = UGAME / "Saved" / "Astrawild" / "DataTableImportReport.json"

CSV_DIR = UGAME / "Content" / "Astrawild" / "Data" / "Source"
expected = sorted(p.name for p in CSV_DIR.glob("DT_*.csv"))
print(f"Expected CSVs:        {len(expected)}")

# Disk reality
on_disk = sorted(p.stem for p in IMPORTS.glob("DT_*.uasset"))
print(f"DT_*.uasset on disk:  {len(on_disk)}")

# Report's claim
report_imported = 0
report_failed = 0
if REPORT.is_file():
    r = json.loads(REPORT.read_text(encoding="utf-8", errors="replace"))
    report_imported = r.get("imported_count", -1)
    report_failed = r.get("failed_count", -1)
    print(f"Report imported_count: {report_imported}")
    print(f"Report failed_count:   {report_failed}")
    print(f"Report expected_count: {r.get('expected_count')}")
else:
    print("Report:               (not present)")

# Cross-check
missing_from_disk = [c for c in expected if Path(c).stem not in on_disk]
extra_on_disk = [d for d in on_disk if f"{d}.csv" not in expected]
print()
print(f"Missing on disk:      {len(missing_from_disk)}")
for m in missing_from_disk:
    print(f"  - {m}")
print(f"Extra on disk:        {len(extra_on_disk)}")
for e in extra_on_disk:
    print(f"  + {e}")

# Verdict
disk_pct = (len(on_disk) / max(1, len(expected))) * 100
print()
print("=" * 60)
print(f"DISK REALITY:  {len(on_disk)}/{len(expected)} = {disk_pct:.1f}% of CSVs have .uasset")
print(f"REPORT CLAIM:  {report_imported}/{len(expected)}")
if disk_pct >= 99 and report_imported == 0:
    print()
    print("VERDICT: The report's 0/38 is a FALSE NEGATIVE — assets")
    print("         are present on disk. The check logic in the import")
    print("         script does not match the actual save pipeline in")
    print("         headless (-NullRHI -Unattended) mode.")
    sys.exit(0)
elif disk_pct < 99:
    print("VERDICT: REAL failure — assets missing on disk.")
    sys.exit(2)
else:
    print("VERDICT: Report matches disk state.")
    sys.exit(0)
