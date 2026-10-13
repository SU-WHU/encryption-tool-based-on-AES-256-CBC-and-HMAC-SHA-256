param(
    [string]$ToolchainBin = 'E:\application\Dev-Cpp\msys64\ucrt64\bin',
    [string]$OpenSSLRoot = 'E:\application\Dev-Cpp\OpenSSL-3.0.22-ucrt64'
)

$ErrorActionPreference = 'Stop'
$savedPath = $env:PATH
$buildDir = Join-Path $PSScriptRoot '..\output\build-check'
$null = New-Item -ItemType Directory -Force -Path $buildDir
$buildDir = (Resolve-Path -LiteralPath $buildDir).Path
$logPath = Join-Path $buildDir 'build.log'
$gcc = Join-Path $ToolchainBin 'gcc.exe'
$sources = @('main.c', 'AES_Encrypt.c', 'CBC.c', 'input_mod.c',
             'key_derivation.c', 'encryption.c', 'authen.c')
$objects = @()
$failed = $false
Set-Content -LiteralPath $logPath -Value 'Compilation check (fresh objects, original sources)' -Encoding UTF8

try {
    $env:PATH = "$ToolchainBin;$savedPath"
    foreach ($source in $sources) {
        $object = Join-Path $buildDir ([IO.Path]::GetFileNameWithoutExtension($source) + '.o')
        $arguments = @('-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-fmax-errors=3',
                       '-I', (Join-Path $OpenSSLRoot 'include'), '-c',
                       (Join-Path $PSScriptRoot $source), '-o', $object)
        Add-Content -LiteralPath $logPath -Value "`nChecking $source"
        & $gcc @arguments >> $logPath 2>&1
        if ($LASTEXITCODE -ne 0) {
            $failed = $true
            Write-Host "FAIL: $source"
        } else {
            $objects += $object
            Write-Host "PASS: $source"
        }
    }
    if (-not $failed) {
        $arguments = $objects + @('-o', (Join-Path $buildDir 'coursework.exe'),
            (Join-Path $OpenSSLRoot 'lib64\libcrypto.a'),
            '-lws2_32', '-lgdi32', '-ladvapi32', '-lcrypt32', '-luser32')
        & $gcc @arguments >> $logPath 2>&1
        $failed = $LASTEXITCODE -ne 0
        if ($failed) { Write-Host 'FAIL: linking' }
        else { Write-Host "PASS: linking -> $buildDir\coursework.exe" }
    } else {
        Write-Host 'Linking skipped because source compilation failed.'
    }
    Write-Host "Log: $logPath"
} finally {
    $env:PATH = $savedPath
}
if ($failed) { exit 1 }
exit 0
