#include "LightThemeFixSettings.h"

namespace
{
	FLinearColor FromSrgb(const uint8 Red, const uint8 Green, const uint8 Blue)
	{
		return FLinearColor::FromSRGBColor(FColor(Red, Green, Blue));
	}
}

ULightThemeFixSettings::ULightThemeFixSettings()
	: bInstallBundledTheme(true)
	, bFollowWindowsTheme(true)
	, WindowsThemeCheckIntervalSeconds(1.0f)
	, bApplyOnlyToLightThemes(true)
	, LightThemeLuminanceThreshold(0.5f)
	, bFixComboBoxRows(true)
	, bFixDockTabs(true)
	, bFixBlueprintGraph(true)
	, bFixContentBrowserTiles(true)
	, bFixTooltipText(true)
	, bFixAnimationTimeline(true)
	, bFixFocusedInputText(true)
	, NodeBodyColor(FromSrgb(0x4B, 0x50, 0x4E))
	, NodeTintedBodyColor(FromSrgb(0x4F, 0x54, 0x52))
	, NodeOutlineColor(FromSrgb(0x30, 0x34, 0x32))
	, NodeTitleTextColor(FromSrgb(0xF4, 0xF4, 0xF4))
	, NodeSubtitleTextColor(FromSrgb(0xDD, 0xE3, 0xB0))
	, BreadcrumbTextColor(FromSrgb(0xF4, 0xF4, 0xF4))
	, NodeCornerRadius(6.0f)
	, NodeOutlineWidth(1.0f)
	, GraphRegularGridColor(FromSrgb(0xD7, 0xDE, 0xE6))
	, GraphRuleGridColor(FromSrgb(0xBE, 0xC9, 0xD4))
	, GraphCenterGridColor(FromSrgb(0x8E, 0xA2, 0xB5))
	, ExecutionWireColor(FromSrgb(0x2B, 0x68, 0xBD))
	, AssetTileHoverColor(FromSrgb(0x3D, 0x73, 0xA7))
	, AssetTileSelectedColor(FromSrgb(0x2F, 0x6F, 0xAE))
	, AssetTileSelectedHoverColor(FromSrgb(0x25, 0x5F, 0x99))
	, AssetTileCornerRadius(4.0f)
	, AnimationTimelineItemColor(FromSrgb(0xC9, 0xD1, 0xD9))
	, AnimationTimelineHeaderColor(FromSrgb(0xB4, 0xC0, 0xCC))
	, AnimationTimelineLabelColor(FromSrgb(0x22, 0x27, 0x2B))
	, FocusedInputTextColor(FromSrgb(0x22, 0x27, 0x2B))
	, TooltipTextColor(FromSrgb(0x2B, 0x2F, 0x33))
	, TooltipRefreshIntervalSeconds(0.25f)
{
}
