@echo off
title ASTRAWILD - Manus AI Live Remote Bridge
color 0B

echo ======================================================================
echo   ASTRAWILD: Starting Live Remote Bridge for Manus AI
echo ======================================================================
echo.
echo [1/2] Launching ASTRAWILD MCP HTTP Server on port 3000...
start /B node "%~dp0Tools\astrawild-mcp\http_server.js" > "%~dp0Tools\astrawild-mcp\bridge.log" 2>&1

timeout /t 2 /nobreak > nul

echo [2/2] Opening Secure HTTPS Tunnel to Cloud via localtunnel...
echo.
echo ----------------------------------------------------------------------
echo   COPY THE URL BELOW AND PASTE IT TO MANUS AI:
echo ----------------------------------------------------------------------
echo.

npx localtunnel --port 3000