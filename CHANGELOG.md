# Changelog

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
