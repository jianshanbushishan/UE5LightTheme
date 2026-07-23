# Changelog

## 2.9.3

- Replaced UE 5.8's hard-coded white Details panel object-name text with the active theme's semantic foreground color.
- Corrected the related Details panel constant-label style and the object-name inline editing state without making the Dark theme text dark.
- Added an independent `Fix Details Panel Text` preference, enabled by default.
- Made runtime theme changes restore every parent-style override and dynamic correction in Dark, then reapply the configured fixes when returning to Light.

## 2.9.2

- Replaced the ineffective top-level Slate window scan with UE 5.8's Actor Browser column-construction extension point.
- Decorated the built-in Label column at row creation time, so PIE-only actor names and icons receive the configured high-contrast color directly.
- Preserved the built-in column's header, search, sorting, rename, drag/drop, disabled-row, ordinary-actor, and dark-theme behavior.

## 2.9.1

- Fixed the Scene Outliner PIE actor correction not reaching the final text and icon paint attributes after Slate rebuilt or refreshed the row.
- Installed dynamic leaf attributes that continuously derive state from the untouched `SActorTreeLabel` foreground, preserving ordinary actor, selection, drag/drop, Level Instance, and dark-theme colors.
- Added a fallback match for the engine's unique hard-coded PIE actor color when concrete Slate debug type information is unavailable.

## 2.9.0

- Added a configurable high-contrast Scene Outliner color for actors that exist only in the Play In Editor world.
- Targeted UE 5.8's private `SActorTreeLabel` paint leaves while preserving its original state attribute, so ordinary actors, selection states, drag/drop states, and the dark theme continue to inherit their normal foregrounds.
- Made the external installer stop when Epic Games Launcher is still running, preventing an apparent successful install whose environment is not inherited by projects launched from that existing process.
- Added `-AllowRunningLauncher` for scripted installations that explicitly handle the required Launcher restart.

## 2.8.1

- Fixed the installer migration from the legacy `%LOCALAPPDATA%\UnrealEngine\ExternalPlugins\UE_{Version}` search path to `%LOCALAPPDATA%\UnrealEngine\{Version}\ExternalPlugins`.
- Normalized installed descriptors to `Installed=true` and `EnabledByDefault=true`, including installations made directly from built source checkouts.
- Allowed the installer to repair and register a plugin that is already located in its default external-plugin directory.
- Added an explicit warning when Epic Games Launcher is still running with an environment that predates the installation.
- Broadcast the Windows environment-change notification so newly launched desktop processes inherit the registered search path.

## 2.8.0

- Made packaged releases installable as precompiled external plugins discovered through `UE_ADDITIONAL_PLUGIN_PATHS`.
- Marked release descriptors as installed and enabled by default so projects no longer need individual plugin references.
- Added a PowerShell installer that uses a versioned per-user plugin directory and preserves existing external-plugin search paths.
- Moved preferences to the user-and-engine `EditorSettings` config so one configuration applies across projects opened by the same engine version.

## 2.7.2

- Added configurable contrast corrections for the Live Coding progress dialog.

## 2.7.1

- Added a display-name fallback when resolving Light and Dark themes, preserving automatic switching for existing user theme files created with an older theme ID.

## 2.7.0

- Added automatic Windows app-theme synchronization between the bundled Light theme and Unreal's default Dark theme.
- Added settings for enabling Windows theme following and configuring its runtime check interval.
- Kept Unreal's application-appearance setting synchronized with automatic theme changes.

## 2.6.0

- Renamed the bundled theme and its installed file to `Light` / `Light.json`.
- Removed legacy branding from the plugin descriptor, source comments, and documentation.

## 2.5.0

- Split runtime tooltip and focused-input work so disabled features no longer incur unnecessary window traversal.
- Guarded the runtime ticker before any settings, Slate, or theme-manager access once editor shutdown begins.
- Kept support for concrete `SSearchBox` and `SFilterSearchBoxImpl` widget type names.
- Removed redundant style enumeration and unused public API.
- Added a reproducible UE BuildPlugin release script, MIT license, contribution guide, and expanded installation and troubleshooting documentation.

## 2.4.1

- Fixed focused text in `SSearchBox` instances used by Output Log, plugin browsing, and most editor search fields.
- Fixed `SFilterSearchBoxImpl`, the specialized search control used by the Scene Outliner and filter bars.
- Accounted for Slate reporting concrete derived widget type names instead of `SEditableTextBox` for search controls.

## 2.4.0

- Moved plugin loading from `PostEngineInit` to `PostDefault`, before MainFrame creates editor widgets that retain pointers to the original white-focused input styles.
- Added an instance-level focused text correction based on the actual `SBorder` brush luminance for text boxes that were created before the style override.
- Preserved adaptive foreground inheritance so dark graph-node input fields are not forced to use dark text.

## 2.3.3

- Fixed numeric input text remaining white while actively editing an `SSpinBox`.
- Added a runtime correction for already-created spin boxes while preserving explicitly dark spin-box styles.

## 2.3.2

- Fixed editable text turning white when a light input field receives keyboard focus.
- Applied the fix to editable text boxes, search boxes, and inline editors only when their focused background is light.
- Added a configurable focused input text color and feature switch.

## 2.3.1

- Added a configurable Animation Editor timeline contrast fix.
- Replaced Persona's hard-coded near-black outliner rows with light item/header colors.
- Replaced the hard-coded white timeline labels and shadows with readable dark text.

## 2.3.0

- Renamed and reorganized the editor module as `LightThemeFixEditor`.
- Split module lifecycle, Slate style overrides, and developer settings into separate files.
- Exposed feature switches, thresholds, colors, timing, corner radius, and outline width in Editor Preferences.
- Added automatic one-time installation of the bundled Light theme.
- Preserved the engine-exit shutdown guard that prevents late `CoreUObject` access.
