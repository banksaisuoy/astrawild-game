#!/bin/bash
cd "C:/Users/saisu/OneDrive - kmutnb.ac.th/Documents/game" || exit 1
"E:/Epic Games/UnrealEngine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  ASTRAWILD.uproject \
  -ExecutePythonScript=Scripts/import_all_datatables.py \
  -NullRHI -NoLoadingScreen -Unattended -nopause -log \
  > /tmp/ue_import.log 2>&1
echo "Exit: $?"
tail -50 /tmp/ue_import.log
