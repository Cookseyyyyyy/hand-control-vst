param(
    [string] $Version = "0.1.0",
    [string] $Configuration = "Release",
    [string] $BuildDir = "$PSScriptRoot/../../build"
)

$ErrorActionPreference = "Stop"

$bundleName = "Hand Control.vst3"
$pluginPath = Resolve-Path (Join-Path $BuildDir "plugin/HandControlPlugin_artefacts/$Configuration/VST3/$bundleName")
if (-not (Test-Path $pluginPath)) {
    throw "VST3 bundle not found at $pluginPath. Build the plugin first."
}

$outDir = Join-Path $PSScriptRoot "out"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Harvest the VST3 bundle contents.
& heat.exe dir $pluginPath `
    -cg HarvestedVST3 `
    -ag `
    -dr HandControlVST3 `
    -sfrag `
    -srd `
    -var var.PluginPath `
    -out (Join-Path $outDir "HandControl.Harvest.wxs")

& candle.exe `
    -dVersion=$Version `
    -dPluginPath=$pluginPath `
    -out (Join-Path $outDir "HandControl.wixobj") `
    (Join-Path $PSScriptRoot "HandControl.wxs")

& candle.exe `
    -dVersion=$Version `
    -dPluginPath=$pluginPath `
    -out (Join-Path $outDir "HandControl.Harvest.wixobj") `
    (Join-Path $outDir "HandControl.Harvest.wxs")

& light.exe `
    -ext WixUIExtension `
    -out (Join-Path $outDir "HandControl-$Version.msi") `
    (Join-Path $outDir "HandControl.wixobj") `
    (Join-Path $outDir "HandControl.Harvest.wixobj")

Write-Host "Built: $(Join-Path $outDir "HandControl-$Version.msi")"
Write-Host ""
Write-Host "To sign for distribution (avoids SmartScreen warnings), run:"
Write-Host "    signtool.exe sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a ""$(Join-Path $outDir "HandControl-$Version.msi")"""
