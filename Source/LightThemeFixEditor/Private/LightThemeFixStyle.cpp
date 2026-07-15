#include "LightThemeFixStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "LightThemeFixSettings.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"

namespace
{
	void SetLightTabForegrounds(FDockTabStyle& TabStyle)
	{
		TabStyle
			.SetNormalForegroundColor(FStyleColors::Foreground)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetActiveForegroundColor(FStyleColors::ForegroundHover)
			.SetForegroundForegroundColor(FStyleColors::ForegroundHover);
	}

	void SetContentBrowserBrushes(FSlateStyleSet& Style, const ULightThemeFixSettings& Settings)
	{
		const float Radius = Settings.AssetTileCornerRadius;
		const FVector4 NameAreaRadius(0.0f, 0.0f, Radius, Radius);

		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.AssetContentHoverBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, FVector4(Radius)));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, FVector4(Radius)));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedHoverBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, FVector4(Radius)));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.NameAreaHoverBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, NameAreaRadius));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, NameAreaRadius));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedHoverBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, NameAreaRadius));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.FolderAreaHoveredBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, Radius));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, Radius));
		Style.Set(
			TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedHoverBackground"),
			new FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, Radius));
	}

	bool HasLightFocusedBackground(const FEditableTextBoxStyle& TextBoxStyle, const float LuminanceThreshold)
	{
		const FWidgetStyle WidgetStyle;
		const FLinearColor FocusedBackground =
			TextBoxStyle.BackgroundImageFocused.GetTint(WidgetStyle) *
			TextBoxStyle.BackgroundColor.GetColor(WidgetStyle);
		return FocusedBackground.GetLuminance() > LuminanceThreshold;
	}

	bool FixFocusedTextColor(FEditableTextBoxStyle& TextBoxStyle, const ULightThemeFixSettings& Settings)
	{
		if (!HasLightFocusedBackground(TextBoxStyle, Settings.LightThemeLuminanceThreshold))
		{
			return false;
		}

		TextBoxStyle.SetFocusedForegroundColor(Settings.FocusedInputTextColor);
		return true;
	}

	bool HasLightSpinBoxBackground(const FSpinBoxStyle& SpinBoxStyle, const float LuminanceThreshold)
	{
		const FWidgetStyle WidgetStyle;
		const FSlateBrush& FocusedBackground = SpinBoxStyle.ActiveBackgroundBrush.IsSet()
			? SpinBoxStyle.ActiveBackgroundBrush
			: SpinBoxStyle.HoveredBackgroundBrush;
		return FocusedBackground.GetTint(WidgetStyle).GetLuminance() > LuminanceThreshold;
	}

	void SetFocusedInputTextColors(FSlateStyleSet& Style, const ULightThemeFixSettings& Settings)
	{
		const ISlateStyle& BaseStyle = FAppStyle::Get();
		for (const FName StyleName : BaseStyle.GetWidgetStyleNames())
		{
			if (BaseStyle.HasWidgetStyle<FSpinBoxStyle>(StyleName))
			{
				FSpinBoxStyle SpinBoxStyle = BaseStyle.GetWidgetStyle<FSpinBoxStyle>(StyleName);
				const bool bIsExplicitDarkStyle = StyleName.ToString().Contains(TEXT("Dark"));
				if (!bIsExplicitDarkStyle &&
					HasLightSpinBoxBackground(SpinBoxStyle, Settings.LightThemeLuminanceThreshold))
				{
					SpinBoxStyle.SetForegroundColor(Settings.FocusedInputTextColor);
					Style.Set(StyleName, SpinBoxStyle);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FEditableTextBoxStyle>(StyleName))
			{
				FEditableTextBoxStyle TextBoxStyle = BaseStyle.GetWidgetStyle<FEditableTextBoxStyle>(StyleName);
				if (FixFocusedTextColor(TextBoxStyle, Settings))
				{
					Style.Set(StyleName, TextBoxStyle);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FSearchBoxStyle>(StyleName))
			{
				FSearchBoxStyle SearchBoxStyle = BaseStyle.GetWidgetStyle<FSearchBoxStyle>(StyleName);
				if (FixFocusedTextColor(SearchBoxStyle.TextBoxStyle, Settings))
				{
					Style.Set(StyleName, SearchBoxStyle);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FInlineEditableTextBlockStyle>(StyleName))
			{
				FInlineEditableTextBlockStyle InlineStyle =
					BaseStyle.GetWidgetStyle<FInlineEditableTextBlockStyle>(StyleName);
				if (FixFocusedTextColor(InlineStyle.EditableTextBoxStyle, Settings))
				{
					Style.Set(StyleName, InlineStyle);
				}
			}
		}
	}
}

void FLightThemeFixStyle::Initialize(const ULightThemeFixSettings& Settings)
{
	if (Style.IsValid())
	{
		return;
	}

	PreviousAppStyleName = FAppStyle::GetAppStyleSetName();
	Style = MakeShared<FSlateStyleSet>(TEXT("LightThemeFixStyle"));
	Style->SetParentStyleName(PreviousAppStyleName);

	if (Settings.bFixComboBoxRows)
	{
		FTableRowStyle ComboBoxRow = FAppStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("ComboBox.Row"));
		ComboBoxRow
			.SetTextColor(FStyleColors::Foreground)
			.SetSelectedTextColor(FStyleColors::Foreground);
		Style->Set(TEXT("ComboBox.Row"), ComboBoxRow);
	}

	if (Settings.bFixFocusedInputText)
	{
		SetFocusedInputTextColors(*Style, Settings);
	}

	if (Settings.bFixBlueprintGraph)
	{
		FTextBlockStyle NodeTitle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("Graph.Node.NodeTitle"));
		NodeTitle.SetColorAndOpacity(Settings.NodeTitleTextColor);

		FTextBlockStyle NodeTitleExtraLines =
			FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("Graph.Node.NodeTitleExtraLines"));
		NodeTitleExtraLines.SetColorAndOpacity(Settings.NodeSubtitleTextColor);

		FInlineEditableTextBlockStyle InlineNodeTitle =
			FAppStyle::Get().GetWidgetStyle<FInlineEditableTextBlockStyle>(TEXT("Graph.Node.NodeTitleInlineEditableText"));
		InlineNodeTitle.SetTextStyle(NodeTitle);

		FTextBlockStyle GraphBreadcrumbText =
			FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("GraphBreadcrumbButtonText"));
		GraphBreadcrumbText.SetColorAndOpacity(Settings.BreadcrumbTextColor);

		Style->Set(TEXT("Graph.Panel.SolidBackground"), new FSlateColorBrush(FStyleColors::Recessed));
		Style->Set(
			TEXT("Graph.Node.Body"),
			new FSlateRoundedBoxBrush(
				Settings.NodeBodyColor,
				Settings.NodeCornerRadius,
				Settings.NodeOutlineColor,
				Settings.NodeOutlineWidth));
		Style->Set(
			TEXT("Graph.Node.TintedBody"),
			new FSlateRoundedBoxBrush(
				Settings.NodeTintedBodyColor,
				Settings.NodeCornerRadius,
				Settings.NodeOutlineColor,
				Settings.NodeOutlineWidth));
		Style->Set(TEXT("Graph.Node.NodeTitle"), NodeTitle);
		Style->Set(TEXT("Graph.Node.NodeTitleExtraLines"), NodeTitleExtraLines);
		Style->Set(TEXT("Graph.Node.NodeTitleInlineEditableText"), InlineNodeTitle);
		Style->Set(TEXT("GraphBreadcrumbButtonText"), GraphBreadcrumbText);
	}

	if (Settings.bFixDockTabs)
	{
		FDockTabStyle DockingTab = FAppStyle::Get().GetWidgetStyle<FDockTabStyle>(TEXT("Docking.Tab"));
		SetLightTabForegrounds(DockingTab);
		Style->Set(TEXT("Docking.Tab"), DockingTab);

		FDockTabStyle DockingMajorTab = FAppStyle::Get().GetWidgetStyle<FDockTabStyle>(TEXT("Docking.MajorTab"));
		SetLightTabForegrounds(DockingMajorTab);
		Style->Set(TEXT("Docking.MajorTab"), DockingMajorTab);
	}

	if (Settings.bFixContentBrowserTiles)
	{
		SetContentBrowserBrushes(*Style, Settings);
	}

	if (Settings.bFixAnimationTimeline)
	{
		FTextBlockStyle TimelineLabel =
			FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("AnimTimeline.Outliner.Label"));
		TimelineLabel
			.SetColorAndOpacity(Settings.AnimationTimelineLabelColor)
			.SetShadowOffset(FVector2f::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent);

		Style->Set(TEXT("AnimTimeline.Outliner.Label"), TimelineLabel);
		Style->Set(TEXT("AnimTimeline.Outliner.ItemColor"), Settings.AnimationTimelineItemColor);
		Style->Set(TEXT("AnimTimeline.Outliner.HeaderColor"), Settings.AnimationTimelineHeaderColor);
	}

	FSlateStyleRegistry::RegisterSlateStyle(*Style);
	FAppStyle::SetAppStyleSet(*Style);
}

void FLightThemeFixStyle::Shutdown()
{
	if (!Style.IsValid())
	{
		return;
	}

	if (FAppStyle::GetAppStyleSetName() == Style->GetStyleSetName())
	{
		FAppStyle::SetAppStyleSetName(PreviousAppStyleName);
	}

	FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
	Style.Reset();
}
