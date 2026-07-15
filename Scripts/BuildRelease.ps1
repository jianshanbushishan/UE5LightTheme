[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string] $EngineRoot,

	[string] $OutputRoot = (Join-Path $PSScriptRoot '..\Dist'),

	[switch] $IncludeDebugSymbols
)

$ErrorActionPreference = 'Stop'

function Get-FullPath([string] $Path) {
	return [System.IO.Path]::GetFullPath($Path).TrimEnd(
		[System.IO.Path]::DirectorySeparatorChar,
		[System.IO.Path]::AltDirectorySeparatorChar)
}

function Assert-ChildPath([string] $Parent, [string] $Child) {
	$parentPath = Get-FullPath $Parent
	$childPath = Get-FullPath $Child
	$prefix = $parentPath + [System.IO.Path]::DirectorySeparatorChar
	if (-not $childPath.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
		throw "Refusing to modify a path outside '$parentPath': $childPath"
	}
}

$pluginRoot = Get-FullPath (Join-Path $PSScriptRoot '..')
$pluginDescriptor = Join-Path $pluginRoot 'LightThemeFix.uplugin'
$runUat = Join-Path (Get-FullPath $EngineRoot) 'Engine\Build\BatchFiles\RunUAT.bat'
$outputRootPath = Get-FullPath $OutputRoot

if (-not (Test-Path -LiteralPath $pluginDescriptor -PathType Leaf)) {
	throw "Plugin descriptor not found: $pluginDescriptor"
}
if (-not (Test-Path -LiteralPath $runUat -PathType Leaf)) {
	throw "RunUAT.bat not found below EngineRoot: $runUat"
}

$descriptor = Get-Content -LiteralPath $pluginDescriptor -Raw | ConvertFrom-Json
$version = $descriptor.VersionName
$engineLabel = 'UE' + (($descriptor.EngineVersion -split '\.')[0..1] -join '.')
$artifactBaseName = "LightThemeFix-$version-$engineLabel-Win64"
$stagingRoot = Join-Path $outputRootPath '.staging'
$packageDirectory = Join-Path $stagingRoot 'LightThemeFix'
$zipPath = Join-Path $outputRootPath ($artifactBaseName + '.zip')
$hashPath = $zipPath + '.sha256'

New-Item -ItemType Directory -Path $outputRootPath -Force | Out-Null

Assert-ChildPath $outputRootPath $stagingRoot
if (Test-Path -LiteralPath $stagingRoot) {
	Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

& $runUat BuildPlugin `
	-Plugin="$pluginDescriptor" `
	-Package="$packageDirectory" `
	-TargetPlatforms=Win64 `
	-Rocket

if ($LASTEXITCODE -ne 0) {
	throw "BuildPlugin failed with exit code $LASTEXITCODE."
}

if (-not $IncludeDebugSymbols) {
	Get-ChildItem -LiteralPath $packageDirectory -Filter '*.pdb' -File -Recurse |
		Remove-Item -Force
}

Assert-ChildPath $outputRootPath $zipPath
Assert-ChildPath $outputRootPath $hashPath
if (Test-Path -LiteralPath $zipPath) {
	Remove-Item -LiteralPath $zipPath -Force
}
if (Test-Path -LiteralPath $hashPath) {
	Remove-Item -LiteralPath $hashPath -Force
}

Compress-Archive -LiteralPath $packageDirectory -DestinationPath $zipPath -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath $hashPath -Value "$hash  $([System.IO.Path]::GetFileName($zipPath))" -Encoding Ascii

Remove-Item -LiteralPath $stagingRoot -Recurse -Force

Write-Host "Release archive: $zipPath"
Write-Host "SHA-256:        $hash"
