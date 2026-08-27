[CmdletBinding()]
param(
    [string]$ProjectRoot = "",
    [string]$PackageDirectory = ""
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Get-Item (Join-Path $PSScriptRoot "..")).FullName
}
if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
    $PackageDirectory = Join-Path $ProjectRoot "Builds\WindowsDevelopment"
}

$validator = Join-Path $PSScriptRoot "Validate_Astrawild.ps1"
if (-not (Test-Path $validator)) {
    throw "Missing validation script: $validator"
}

Write-Host "Running source/content validation before packaging..." -ForegroundColor Cyan
& $validator -ProjectRoot $ProjectRoot
if ($LASTEXITCODE -ne 0) {
    throw "Source/content validation failed with exit code $LASTEXITCODE"
}

Write-Host "Running Unreal compile check and Development package..." -ForegroundColor Cyan
& $validator -ProjectRoot $ProjectRoot -TryUnreal -Package -PackageDirectory $PackageDirectory
if ($LASTEXITCODE -ne 0) {
    throw "Unreal compile/package step failed with exit code $LASTEXITCODE"
}

Write-Host "Package workflow completed. Record the result in Docs\BUILD_STATUS.md." -ForegroundColor Green
