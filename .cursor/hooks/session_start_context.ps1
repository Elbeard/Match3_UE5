param()

$ErrorActionPreference = "SilentlyContinue"

$projectRoot = (Get-Location).Path
$contextDir = Join-Path $projectRoot ".cursor/context"
if (-not (Test-Path $contextDir)) {
    New-Item -ItemType Directory -Path $contextDir | Out-Null
}

$dailyFile = Join-Path $contextDir ("CONTEXT_" + (Get-Date -Format "yyyy-MM-dd") + ".txt")
if (-not (Test-Path $dailyFile)) {
    New-Item -ItemType File -Path $dailyFile | Out-Null
}

$docsPath = Join-Path $projectRoot "MATCH3_PROJECT_DOCUMENTATION.txt"
$sessionSummaryPath = Join-Path $contextDir "SESSION_CONTEXT_AUTO.txt"

$summary = @()
$summary += "AUTO SESSION CONTEXT"
$summary += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$summary += ""
$summary += "Read these first in future requests:"
$summary += "- MATCH3_PROJECT_DOCUMENTATION.txt"
$summary += "- .cursor/context/CONTEXT_$(Get-Date -Format 'yyyy-MM-dd').txt"
$summary += ""

if (Test-Path $docsPath) {
    $summary += "Documentation file found."
} else {
    $summary += "Documentation file is missing: MATCH3_PROJECT_DOCUMENTATION.txt"
}

$summary += ""
$summary += "Latest context entries:"
Get-Content -Path $dailyFile -Tail 30 | ForEach-Object { $summary += $_ }

Set-Content -Path $sessionSummaryPath -Value $summary -Encoding UTF8

# Minimal valid JSON response.
Write-Output "{""permission"":""allow""}"
