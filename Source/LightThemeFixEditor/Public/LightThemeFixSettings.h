#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LightThemeFixSettings.generated.h"

/** User-configurable contrast fixes for light Unreal Editor themes. */
UCLASS(config=EditorPerProjectUserSettings, defaultconfig, meta=(DisplayName="Light Theme Fix"))
class LIGHTTHEMEFIXEDITOR_API ULightThemeFixSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULightThemeFixSettings();

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Light Theme Fix"); }

	/** Copy the bundled Light theme to the current user's Unreal theme directory. */
	UPROPERTY(config, EditAnywhere, Category="Theme", meta=(ConfigRestartRequired=true))
	bool bInstallBundledTheme;

	/** Apply graph-color and runtime contrast corrections only when the active theme background is light. */
	UPROPERTY(config, EditAnywhere, Category="Theme")
	bool bApplyOnlyToLightThemes;

	/** Relative luminance above which a theme is treated as light. */
	UPROPERTY(config, EditAnywhere, Category="Theme", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LightThemeLuminanceThreshold;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixComboBoxRows;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixDockTabs;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixBlueprintGraph;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixContentBrowserTiles;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixTooltipText;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixAnimationTimeline;

	UPROPERTY(config, EditAnywhere, Category="Features", meta=(ConfigRestartRequired=true))
	bool bFixFocusedInputText;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor NodeBodyColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor NodeTintedBodyColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor NodeOutlineColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor NodeTitleTextColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor NodeSubtitleTextColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor BreadcrumbTextColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(ClampMin="0.0", ClampMax="32.0", ConfigRestartRequired=true))
	float NodeCornerRadius;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(ClampMin="0.0", ClampMax="8.0", ConfigRestartRequired=true))
	float NodeOutlineWidth;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel))
	FLinearColor GraphRegularGridColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel))
	FLinearColor GraphRuleGridColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel))
	FLinearColor GraphCenterGridColor;

	UPROPERTY(config, EditAnywhere, Category="Blueprint Graph", meta=(HideAlphaChannel))
	FLinearColor ExecutionWireColor;

	UPROPERTY(config, EditAnywhere, Category="Content Browser", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AssetTileHoverColor;

	UPROPERTY(config, EditAnywhere, Category="Content Browser", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AssetTileSelectedColor;

	UPROPERTY(config, EditAnywhere, Category="Content Browser", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AssetTileSelectedHoverColor;

	UPROPERTY(config, EditAnywhere, Category="Content Browser", meta=(ClampMin="0.0", ClampMax="32.0", ConfigRestartRequired=true))
	float AssetTileCornerRadius;

	/** Background used by ordinary rows in the Animation Editor timeline outliner. */
	UPROPERTY(config, EditAnywhere, Category="Animation Timeline", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AnimationTimelineItemColor;

	/** Background used by group/header rows such as Notifies, Curves, and Attributes. */
	UPROPERTY(config, EditAnywhere, Category="Animation Timeline", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AnimationTimelineHeaderColor;

	UPROPERTY(config, EditAnywhere, Category="Animation Timeline", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor AnimationTimelineLabelColor;

	/** Text color used while typing in input fields with a light focused background. */
	UPROPERTY(config, EditAnywhere, Category="Input Fields", meta=(HideAlphaChannel, ConfigRestartRequired=true))
	FLinearColor FocusedInputTextColor;

	UPROPERTY(config, EditAnywhere, Category="Tooltips", meta=(HideAlphaChannel))
	FLinearColor TooltipTextColor;

	/** Refresh interval shared by runtime tooltip and focused-input corrections. */
	UPROPERTY(config, EditAnywhere, Category="Runtime Contrast", meta=(DisplayName="Runtime Contrast Refresh Interval", ClampMin="0.05", ClampMax="5.0", Units="s", ConfigRestartRequired=true))
	float TooltipRefreshIntervalSeconds;
};
