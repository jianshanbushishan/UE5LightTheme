[CmdletBinding()]
param(
	[string] $InstallRoot,

	[switch] $SkipUserEnvironmentRegistration,

	[switch] $AllowRunningLauncher
)

$ErrorActionPreference = 'Stop'

function Get-FullPath([string] $Path) {
	return [System.IO.Path]::GetFullPath($Path).TrimEnd(
		[System.IO.Path]::DirectorySeparatorChar,
		[System.IO.Path]::AltDirectorySeparatorChar)
}

function Get-EnvironmentPaths([string] $Value) {
	if ([string]::IsNullOrWhiteSpace($Value)) {
		return @()
	}

	return @($Value.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries) |
		ForEach-Object { $_.Trim() } |
		Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Test-PathListContains([string[]] $Paths, [string] $Candidate) {
	$candidatePath = Get-FullPath $Candidate
	foreach ($pathEntry in $Paths) {
		try {
			if ((Get-FullPath $pathEntry).Equals($candidatePath, [System.StringComparison]::OrdinalIgnoreCase)) {
				return $true
			}
		}
		catch {
			# Preserve malformed or unavailable existing entries without treating
			# them as a match for the path being installed.
		}
	}
	return $false
}

function Test-PathsEqual([string] $First, [string] $Second) {
	try {
		return (Get-FullPath $First).Equals(
			(Get-FullPath $Second),
			[System.StringComparison]::OrdinalIgnoreCase)
	}
	catch {
		return $false
	}
}

function Set-InstalledDescriptorFlags([string] $DescriptorPath) {
	$installedDescriptor = Get-Content -LiteralPath $DescriptorPath -Raw | ConvertFrom-Json
	$installedDescriptor | Add-Member -MemberType NoteProperty -Name EnabledByDefault -Value $true -Force
	$installedDescriptor | Add-Member -MemberType NoteProperty -Name Installed -Value $true -Force
	$installedDescriptor | ConvertTo-Json -Depth 100 |
		Set-Content -LiteralPath $DescriptorPath -Encoding UTF8
}

function Send-EnvironmentChangedNotification {
	if ($env:OS -ne 'Windows_NT') {
		return
	}

	if ($null -eq ('LightThemeFix.NativeEnvironment' -as [Type])) {
		Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace LightThemeFix
{
    public static class NativeEnvironment
    {
        [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr SendMessageTimeout(
            IntPtr window,
            uint message,
            UIntPtr wParam,
            string lParam,
            uint flags,
            uint timeoutMilliseconds,
            out UIntPtr result);
    }
}
'@
	}

	[UIntPtr] $notificationResult = [UIntPtr]::Zero
	[void] [LightThemeFix.NativeEnvironment]::SendMessageTimeout(
		[ IntPtr ] 0xffff,
		0x001a,
		[UIntPtr]::Zero,
		'Environment',
		0x0002,
		5000,
		[ref] $notificationResult)
}

$pluginRoot = Get-FullPath (Join-Path $PSScriptRoot '..')
$pluginDescriptor = Join-Path $pluginRoot 'LightThemeFix.uplugin'
if (-not (Test-Path -LiteralPath $pluginDescriptor -PathType Leaf)) {
	throw "Plugin descriptor not found: $pluginDescriptor"
}

$descriptor = Get-Content -LiteralPath $pluginDescriptor -Raw | ConvertFrom-Json
if ([string]::IsNullOrWhiteSpace($descriptor.EngineVersion)) {
	throw 'The plugin descriptor must declare EngineVersion for an external installation.'
}

$engineVersionParts = $descriptor.EngineVersion.Split('.')
if ($engineVersionParts.Count -lt 2) {
	throw "Invalid EngineVersion in plugin descriptor: $($descriptor.EngineVersion)"
}
$supportedEngineVersion = "$($engineVersionParts[0]).$($engineVersionParts[1])"
$localAppData = [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)
$usingDefaultInstallRoot = [string]::IsNullOrWhiteSpace($InstallRoot)

if ($usingDefaultInstallRoot) {
	$InstallRoot = Join-Path $localAppData "UnrealEngine\$supportedEngineVersion\ExternalPlugins"
}
$installRootPath = Get-FullPath $InstallRoot
$destination = Join-Path $installRootPath 'LightThemeFix'
$legacyInstallRoot = Join-Path $localAppData "UnrealEngine\ExternalPlugins\UE_$supportedEngineVersion"
$isAlreadyAtDestination = Test-PathsEqual $pluginRoot $destination

$editorProcess = Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue
if ($null -ne $editorProcess) {
	throw 'Close all Unreal Editor instances before installing or updating Light Theme Fix.'
}

$launcherProcess = Get-Process -Name 'EpicGamesLauncher' -ErrorAction SilentlyContinue
if ($null -ne $launcherProcess -and
	-not $SkipUserEnvironmentRegistration -and
	-not $AllowRunningLauncher) {
	throw 'Exit Epic Games Launcher completely, including its tray process, before installing. A running launcher cannot inherit the new UE_ADDITIONAL_PLUGIN_PATHS value and projects opened from it will not load Light Theme Fix. Pass -AllowRunningLauncher only if you will restart the launcher before opening Unreal Editor.'
}

$binaryPattern = 'UnrealEditor-LightThemeFixEditor*.dll'
if (-not (Get-ChildItem -LiteralPath (Join-Path $pluginRoot 'Binaries\Win64') -Filter $binaryPattern -File -ErrorAction SilentlyContinue)) {
	throw 'No precompiled Win64 editor binary was found. Install from a release archive built for this Unreal Engine version.'
}

$backupRoot = Join-Path $localAppData 'UnrealEngine\ExternalPluginBackups'
$backupDirectory = $null

if ($isAlreadyAtDestination) {
	Set-InstalledDescriptorFlags $pluginDescriptor
}
else {
	New-Item -ItemType Directory -Path $installRootPath -Force | Out-Null
	$stagingDirectory = Join-Path $installRootPath ('.LightThemeFix.install-' + [Guid]::NewGuid().ToString('N'))

	try {
		New-Item -ItemType Directory -Path $stagingDirectory -Force | Out-Null
		$excludedNames = @('.git', 'Dist', 'Intermediate', 'Saved')
		Get-ChildItem -LiteralPath $pluginRoot -Force |
			Where-Object { $excludedNames -notcontains $_.Name } |
			Copy-Item -Destination $stagingDirectory -Recurse -Force

		$stagedDescriptorPath = Join-Path $stagingDirectory 'LightThemeFix.uplugin'
		if (-not (Test-Path -LiteralPath $stagedDescriptorPath -PathType Leaf)) {
			throw 'Staged installation is missing LightThemeFix.uplugin.'
		}

		# Source checkouts intentionally use Installed=false for local development.
		# Normalize the installed copy so direct installation from a built checkout
		# has the same descriptor flags as a packaged release.
		Set-InstalledDescriptorFlags $stagedDescriptorPath

		if (Test-Path -LiteralPath $destination) {
			New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
			$backupDirectory = Join-Path $backupRoot ('LightThemeFix-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
			Move-Item -LiteralPath $destination -Destination $backupDirectory
		}

		Move-Item -LiteralPath $stagingDirectory -Destination $destination
	}
	catch {
		if (Test-Path -LiteralPath $stagingDirectory) {
			Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
		}
		if ($null -ne $backupDirectory -and
			(Test-Path -LiteralPath $backupDirectory) -and
			-not (Test-Path -LiteralPath $destination)) {
			Move-Item -LiteralPath $backupDirectory -Destination $destination
		}
		throw
	}
}

if (-not $SkipUserEnvironmentRegistration) {
	$userValue = [Environment]::GetEnvironmentVariable('UE_ADDITIONAL_PLUGIN_PATHS', 'User')
	$userPaths = @(Get-EnvironmentPaths $userValue)
	if ($usingDefaultInstallRoot) {
		$userPaths = @($userPaths | Where-Object { -not (Test-PathsEqual $_ $legacyInstallRoot) })
	}
	if (-not (Test-PathListContains $userPaths $installRootPath)) {
		$userPaths += $installRootPath
	}
	[Environment]::SetEnvironmentVariable(
		'UE_ADDITIONAL_PLUGIN_PATHS',
		($userPaths -join ';'),
		'User')
	Send-EnvironmentChangedNotification
}

$processValue = [Environment]::GetEnvironmentVariable('UE_ADDITIONAL_PLUGIN_PATHS', 'Process')
$processPaths = @(Get-EnvironmentPaths $processValue)
if (-not (Test-PathListContains $processPaths $installRootPath)) {
	$processPaths += $installRootPath
	[Environment]::SetEnvironmentVariable(
		'UE_ADDITIONAL_PLUGIN_PATHS',
		($processPaths -join ';'),
		'Process')
}

Write-Host "Installed plugin:             $destination"
Write-Host "Supported Unreal version:     $supportedEngineVersion"
Write-Host "UE_ADDITIONAL_PLUGIN_PATHS: $installRootPath"
if ($null -ne $backupDirectory) {
	Write-Host "Previous version backup:      $backupDirectory"
}
if ($SkipUserEnvironmentRegistration) {
	Write-Host 'The user environment variable was not changed. Set it in the process that launches Unreal Editor.'
}
else {
	if ($null -ne $launcherProcess) {
		Write-Warning 'Epic Games Launcher is running with its previous environment. Exit it completely, including its tray process, before reopening Unreal Editor.'
	}
	Write-Host 'Restart Unreal Editor and the process used to launch it so they inherit the updated user environment.'
}
