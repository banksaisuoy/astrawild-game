@echo off
setlocal
set UE_ROOT=E:\Epic Games\UnrealEngine
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\ASTRAWILD.uproject
set SCRIPT_PATH=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\Scripts\debug_structs3.py
set LOG=%~dp0debug_structs3.log

"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -run=pythonscript -script="%SCRIPT_PATH%" -unattended -nopause -nullrhi -nosplash -log -stdout -FullStdOutLogOutput -ABSLOG="%LOG%" -NoLiveCoding
echo RC=%ERRORLEVEL%
endlocal
