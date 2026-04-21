param()

$ErrorActionPreference = "SilentlyContinue"

$projectRoot = (Get-Location).Path
$contextDir = Join-Path $projectRoot ".cursor/context"
if (-not (Test-Path $contextDir)) {
    New-Item -ItemType Directory -Path $contextDir | Out-Null
}

$dateTag = Get-Date -Format "yyyy-MM-dd"
$logPath = Join-Path $contextDir ("CONTEXT_" + $dateTag + ".txt")

$raw = [Console]::In.ReadToEnd()
$event = $null
if (-not [string]::IsNullOrWhiteSpace($raw)) {
    try { $event = $raw | ConvertFrom-Json } catch {}
}

$time = Get-Date -Format "HH:mm:ss"
$toolName = "afterFileEdit"
$files = @()

if ($event -ne $null) {
    if ($event.tool_name) { $toolName = [string]$event.tool_name }
    if ($event.files) {
        foreach ($f in $event.files) {
            if ($f.path) { $files += [string]$f.path }
            elseif ($f) { $files += [string]$f }
        }
    } elseif ($event.path) {
        $files += [string]$event.path
    }
}

$files = $files | Select-Object -Unique
if ($files.Count -eq 0) { $files = @("(path not provided by event payload)") }

$lines = @()
$lines += "[$time] EDIT EVENT ($toolName)"
foreach ($p in $files) {
    $lines += "- $p"
}
$lines += ""

Add-Content -Path $logPath -Value $lines -Encoding UTF8

# Minimal valid JSON response.
Write-Output "{""permission"":""allow""}"
