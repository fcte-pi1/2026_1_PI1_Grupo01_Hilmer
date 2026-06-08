# Testes Unity via PlatformIO (ambiente native)
# Uso: .\scripts\run_platformio_tests.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$FirmwareDir = Join-Path $Root "src\firmware\micromouse"
$ReportsDir = Join-Path $Root "reports"

$PioCandidates = @(
    (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"),
    (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe")
)

$Pio = $null
foreach ($candidate in $PioCandidates) {
    if (Test-Path $candidate) {
        $Pio = $candidate
        break
    }
}

if (-not $Pio) {
    $cmd = Get-Command pio -ErrorAction SilentlyContinue
    if ($cmd) { $Pio = $cmd.Source }
}

if (-not $Pio) {
    [Console]::Error.WriteLine("PlatformIO nao encontrado.")
    [Console]::Error.WriteLine("")
    [Console]::Error.WriteLine("Instale a extensao PlatformIO IDE ou: pip install platformio")
    [Console]::Error.WriteLine("Caminho esperado: $env:USERPROFILE\.platformio\penv\Scripts\pio.exe")
    exit 1
}

[Console]::WriteLine("Usando: $Pio")
[Console]::WriteLine("Diretorio: $FirmwareDir")
[Console]::WriteLine("")

if (-not (Test-Path $ReportsDir)) {
    New-Item -ItemType Directory -Path $ReportsDir | Out-Null
}

$JUnitPath = Join-Path $ReportsDir "unit-floodfill.xml"
Push-Location $FirmwareDir

& $Pio test -e native -v --junit-output-path $JUnitPath
$code = $LASTEXITCODE

Pop-Location

if ($code -eq 0) {
    [Console]::WriteLine("")
    [Console]::WriteLine("Testes OK. Relatorio JUnit: $JUnitPath")
} else {
    [Console]::Error.WriteLine("")
    [Console]::Error.WriteLine("Testes falharam (codigo $code)")
}

exit $code
