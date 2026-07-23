#Requires -Version 5.1
<#
.SYNOPSIS
    Checks whether SNES asset sources are newer than generated outputs.
.DESCRIPTION
    Derives the list of expected generated assets directly from build_assets.bat,
    then compares the newest source write time against the oldest generated output
    write time. If sources are newer, writes a stale marker and report.
#>
[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [string]$StateFile = "",
    [string]$ReportFile = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Definition }
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $scriptDir }
if (-not $StateFile) { $StateFile = Join-Path $scriptDir ".assets_stale" }
if (-not $ReportFile) { $ReportFile = Join-Path $scriptDir "asset_watcher_report.txt" }

$buildAssetsPath = Join-Path $RepoRoot "build_assets.bat"
if (-not (Test-Path $buildAssetsPath)) {
    throw "build_assets.bat not found at $buildAssetsPath"
}

$bat = Get-Content $buildAssetsPath -Raw

# Resolve simple variable assignments used in destination paths.
$varMap = @{}
foreach ($m in [regex]::Matches($bat, 'set\s+(\w+)=(.+)')) {
    $varMap[$m.Groups[1].Value] = $m.Groups[2].Value.Trim()
}

function Resolve-AssetVar {
    param([string]$value)
    foreach ($key in $varMap.Keys) {
        $value = $value -replace "%$key%", $varMap[$key]
    }
    return $value.TrimStart('.\').Replace('/', '\')
}

# Collect explicit destination files from build_assets.bat:
#   superfamiconv ... -d <path>
#   python ... -o <path>
# Ignore batch loop metavariables like %%~nF_p.bin.
$expectedOutputs = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($m in [regex]::Matches($bat, '-d\s+(.+?)(?:\s|$)')) {
    $candidate = Resolve-AssetVar $m.Groups[1].Value
    if ($candidate -notmatch '%') { [void]$expectedOutputs.Add($candidate) }
}
foreach ($m in [regex]::Matches($bat, '-o\s+(.+?)(?:\s|$)')) {
    $candidate = Resolve-AssetVar $m.Groups[1].Value
    if ($candidate -notmatch '%') { [void]$expectedOutputs.Add($candidate) }
}

$sourceSpecs = @(
    @{ Path = "bg"; Filter = "*.png"; Recurse = $false },
    @{ Path = "maps"; Filter = "*.png"; Recurse = $false },
    @{ Path = "maps"; Filter = "*.tmj"; Recurse = $false },
    @{ Path = "maps"; Filter = "*.tmx"; Recurse = $false },
    @{ Path = "sprites"; Filter = "*.png"; Recurse = $false },
    @{ Path = "sprites/boss"; Filter = "*.png"; Recurse = $false },
    @{ Path = "ui"; Filter = "*.png"; Recurse = $false },
    @{ Path = "error"; Filter = "*.png"; Recurse = $false },
    @{ Path = "splash"; Filter = "*.png"; Recurse = $false },
    @{ Path = "cutscene"; Filter = "*.png"; Recurse = $true }
)

function Get-MaxWriteTime {
    param([array]$Specs, [ref]$OutFile = $null)
    $max = $null
    $maxFile = $null
    foreach ($spec in $Specs) {
        $fullPath = Join-Path $RepoRoot $spec.Path
        if (-not (Test-Path $fullPath)) { continue }
        $files = Get-ChildItem -Path $fullPath -Filter $spec.Filter `
            -Recurse:$spec.Recurse -File -ErrorAction SilentlyContinue
        foreach ($f in $files) {
            if ($null -eq $max -or $f.LastWriteTimeUtc -gt $max) {
                $max = $f.LastWriteTimeUtc
                $maxFile = $f.FullName
            }
        }
    }
    if ($null -ne $OutFile) { $OutFile.Value = $maxFile }
    return $max
}

function Get-MinWriteTime {
    param([ref]$OutFile = $null)
    $min = $null
    $minFile = $null
    foreach ($rel in $expectedOutputs) {
        $fullPath = Join-Path $RepoRoot $rel
        if (-not (Test-Path $fullPath)) {
            $min = [datetime]::MinValue
            $minFile = "MISSING: $fullPath"
            break
        }
        $f = Get-Item $fullPath
        if ($null -eq $min -or $f.LastWriteTimeUtc -lt $min) {
            $min = $f.LastWriteTimeUtc
            $minFile = $f.FullName
        }
    }
    if ($null -ne $OutFile) { $OutFile.Value = $minFile }
    return $min
}

$now = [DateTimeOffset]::UtcNow.ToString("yyyy-MM-dd HH:mm:ss 'UTC'")
$newestSourceFile = $null
$oldestOutputFile = $null
$newestSource = Get-MaxWriteTime -Specs $sourceSpecs -OutFile ([ref]$newestSourceFile)
$oldestOutput = Get-MinWriteTime -OutFile ([ref]$oldestOutputFile)

$stale = $false
$reason = ""

if ($null -eq $newestSource) {
    $reason = "No asset source files found."
} elseif ($null -eq $oldestOutput) {
    $stale = $true
    $reason = "Generated asset outputs are missing."
} elseif ($oldestOutputFile -like "MISSING:*") {
    $stale = $true
    $reason = "Expected generated output is missing."
} elseif ($newestSource -gt $oldestOutput) {
    $stale = $true
    $reason = "Source assets are newer than generated outputs."
} else {
    $reason = "Generated outputs are up to date with source assets."
}

if ($stale) {
    "STALE" | Set-Content -Path $StateFile -NoNewline
} else {
    if (Test-Path $StateFile) { Remove-Item $StateFile -Force }
}

$report = @"
Asset Watcher Report
Generated: $now
Repository: $RepoRoot
Newest source: $newestSource
  File: $newestSourceFile
Oldest output: $oldestOutput
  File: $oldestOutputFile
Status: $(if ($stale) { "STALE" } else { "OK" })
Reason: $reason
"@
$report | Set-Content -Path $ReportFile

Write-Host $report
exit 0
