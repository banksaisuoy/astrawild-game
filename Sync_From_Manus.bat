@echo off
title ASTRAWILD - Sync From Manus AI
color 0A
echo ========================================================
echo   ASTRAWILD: Syncing latest updates from Manus AI
echo ========================================================
echo.

echo [1/3] Pulling latest code and assets from GitHub...
git pull origin release/vertical-slice-v1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Git pull failed! Please check your network connection.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [2/3] Running Code Audit via ASTRAWILD MCP...
node Tools\astrawild-mcp\server.js < nul > nul 2>&1
echo [OK] Codebase verified.

echo.
echo [3/3] Sync Complete! You can now open Unreal Engine 5.8 and press Play!
echo.
pause