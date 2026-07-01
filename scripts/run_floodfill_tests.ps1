# Executa a suíte completa de testes do FloodFill
# Uso: .\scripts\run_floodfill_tests.ps1 [--scenario=CT01]

param(
    [string]$Scenario = ""
)

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $PSScriptRoot
$Args = @()

if ($Scenario) {
    $Args += "--scenario=$Scenario"
}

Write-Host "Executando suíte FloodFill em $Root"
node "$Root\scripts\run_floodfill_tests.js" @Args
exit $LASTEXITCODE
