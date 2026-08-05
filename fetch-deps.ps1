# fetch-deps.ps1 - Download and extract build tools for GNXEngine
param(
    [string]$Url = "https://github.com/zhouxuguang/engine-dep/releases/download/1.0/ispc.zip",
    [string]$DestDir = "$PSScriptRoot\buildtools"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "Continue"

$archiveName = [System.IO.Path]::GetFileName($Url)
$archivePath = "$env:TEMP\$archiveName"

Write-Host "============================================"
Write-Host " GNXEngine Build Tools Fetcher"
Write-Host "============================================"
Write-Host ""

# Create destination directory
if (-not (Test-Path $DestDir)) {
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
    Write-Host "[*] Created: $DestDir"
}

# Download
Write-Host "[1] Downloading $archiveName ..."
try {
    Invoke-WebRequest -Uri $Url -OutFile $archivePath -ErrorAction Stop
    Write-Host "    Done: $archivePath"
} catch {
    Write-Host "    ERROR: Download failed: $_" -ForegroundColor Red
    exit 1
}

# Extract (zip)
Write-Host "[2] Extracting to $DestDir ..."
try {
    $shell = New-Object -ComObject Shell.Application
    $zipFolder = $shell.NameSpace($archivePath)
    $destFolder = $shell.NameSpace($DestDir)
    $destFolder.CopyHere($zipFolder.Items(), 1556)  # 1560 = 16+4+512+1024, suppress all dialogs
    # Wait until extraction completes (poll for file count stability)
    Start-Sleep -Milliseconds 500
    Write-Host "    Done."
} catch {
    Write-Host "    ERROR: Extraction failed: $_" -ForegroundColor Red
    Remove-Item $archivePath -Force -ErrorAction SilentlyContinue
    exit 1
}

# Cleanup
Remove-Item $archivePath -Force
Write-Host "    Done."

Write-Host ""
Write-Host "============================================"
Write-Host " Build tools installed to: $DestDir"
Write-Host "============================================"
