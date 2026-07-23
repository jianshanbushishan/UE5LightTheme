#include "LightThemeFixStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "LightThemeFixSettings.h"
#include "Application/SlateApplicationBase.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"

namespace
{
	template<typename StyleType>
	StyleType& GetMutableWidgetStyle(FSlateStyleSet& Style, const FName StyleName)
	{
		return const_cast<StyleType&>(Style.GetWidgetStyle<StyleType>(StyleName));
	}

	FSlateBrush& GetMutableBrush(FSlateStyleSet& Style, const FName StyleName)
	{
		return *const_cast<FSlateBrush*>(Style.GetBrush(StyleName));
	}

	FLinearColor& GetMutableColor(FSlateStyleSet& Style, const FName StyleName)
	{
		return const_cast<FLinearColor&>(Style.GetColor(StyleName));
	}

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

	void SetFocusedInputTextColors(
		FSlateStyleSet& Style,
		const ULightThemeFixSettings& Settings,
		TArray<FName>& SpinBoxStyleNames,
		TArray<FName>& EditableTextBoxStyleNames,
		TArray<FName>& SearchBoxStyleNames,
		TArray<FName>& InlineTextStyleNames)
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
					SpinBoxStyleNames.Add(StyleName);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FEditableTextBoxStyle>(StyleName))
			{
				FEditableTextBoxStyle TextBoxStyle = BaseStyle.GetWidgetStyle<FEditableTextBoxStyle>(StyleName);
				if (FixFocusedTextColor(TextBoxStyle, Settings))
				{
					Style.Set(StyleName, TextBoxStyle);
					EditableTextBoxStyleNames.Add(StyleName);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FSearchBoxStyle>(StyleName))
			{
				FSearchBoxStyle SearchBoxStyle = BaseStyle.GetWidgetStyle<FSearchBoxStyle>(StyleName);
				if (FixFocusedTextColor(SearchBoxStyle.TextBoxStyle, Settings))
				{
					Style.Set(StyleName, SearchBoxStyle);
					SearchBoxStyleNames.Add(StyleName);
				}
			}
			else if (BaseStyle.HasWidgetStyle<FInlineEditableTextBlockStyle>(StyleName))
			{
				FInlineEditableTextBlockStyle InlineStyle =
					BaseStyle.GetWidgetStyle<FInlineEditableTextBlockStyle>(StyleName);
				if (FixFocusedTextColor(InlineStyle.EditableTextBoxStyle, Settings))
				{
					Style.Set(StyleName, InlineStyle);
					InlineTextStyleNames.Add(StyleName);
				}
			}
		}
	}

	template<typename StyleType>
	void RestoreWidgetStyles(
		FSlateStyleSet& Style,
		const ISlateStyle& BaseStyle,
		const TArray<FName>& StyleNames)
	{
		for (const FName StyleName : StyleNames)
		{
			GetMutableWidgetStyle<StyleType>(Style, StyleName) =
				BaseStyle.GetWidgetStyle<StyleType>(StyleName);
		}
	}

	void SetDetailsPanelTextStyles(FSlateStyleSet& Style)
	{
		const ISlateStyle& BaseStyle = FAppStyle::Get();

		// UE 5.8 derives both styles from a local NameStyle whose display and
		// edit foregrounds are hard-coded to EStyleColor::White. Use the theme
		// token so the same override remains readable in both Light and Dark.
		FInlineEditableTextBlockStyle ObjectNameStyle =
			BaseStyle.GetWidgetStyle<FInlineEditableTextBlockStyle>(TEXT("DetailsView.NameTextBlockStyle"));
		ObjectNameStyle.TextStyle.SetColorAndOpacity(FStyleColors::Foreground);
		ObjectNameStyle.EditableTextBoxStyle
			.SetForegroundColor(FStyleColors::Foreground)
			.SetFocusedForegroundColor(FStyleColors::Foreground);
		Style.Set(TEXT("DetailsView.NameTextBlockStyle"), ObjectNameStyle);

		FTextBlockStyle ConstantTextStyle =
			BaseStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("DetailsView.ConstantTextBlockStyle"));
		ConstantTextStyle.SetColorAndOpacity(FStyleColors::Foreground);
		Style.Set(TEXT("DetailsView.ConstantTextBlockStyle"), ConstantTextStyle);
	}

	void ApplyFocusedInputTextColors(
		FSlateStyleSet& Style,
		const ULightThemeFixSettings& Settings,
		const TArray<FName>& SpinBoxStyleNames,
		const TArray<FName>& EditableTextBoxStyleNames,
		const TArray<FName>& SearchBoxStyleNames,
		const TArray<FName>& InlineTextStyleNames)
	{
		for (const FName StyleName : SpinBoxStyleNames)
		{
			FSpinBoxStyle& SpinBoxStyle = GetMutableWidgetStyle<FSpinBoxStyle>(Style, StyleName);
			if (HasLightSpinBoxBackground(SpinBoxStyle, Settings.LightThemeLuminanceThreshold))
			{
				SpinBoxStyle.SetForegroundColor(Settings.FocusedInputTextColor);
			}
		}

		for (const FName StyleName : EditableTextBoxStyleNames)
		{
			FixFocusedTextColor(
				GetMutableWidgetStyle<FEditableTextBoxStyle>(Style, StyleName),
				Settings);
		}

		for (const FName StyleName : SearchBoxStyleNames)
		{
			FixFocusedTextColor(
				GetMutableWidgetStyle<FSearchBoxStyle>(Style, StyleName).TextBoxStyle,
				Settings);
		}

		for (const FName StyleName : InlineTextStyleNames)
		{
			FixFocusedTextColor(
				GetMutableWidgetStyle<FInlineEditableTextBlockStyle>(Style, StyleName).EditableTextBoxStyle,
				Settings);
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
		SetFocusedInputTextColors(
			*Style,
			Settings,
			FocusedSpinBoxStyleNames,
			FocusedEditableTextBoxStyleNames,
			FocusedSearchBoxStyleNames,
			FocusedInlineTextStyleNames);
	}

	if (Settings.bFixDetailsPanelText)
	{
		SetDetailsPanelTextStyles(*Style);
	}

	Style->Set(
		TEXT("LightThemeFix.LiveCoding.ProgressDialogBackground"),
		new FSlateRoundedBoxBrush(
			Settings.LiveCodingDialogBackgroundColor,
			Settings.LiveCodingDialogCornerRadius));

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

void FLightThemeFixStyle::ApplyTheme(
	const ULightThemeFixSettings& Settings,
	const bool bEnableLightThemeFixes)
{
	if (!Style.IsValid())
	{
		return;
	}

	const ISlateStyle* BaseStyle = FSlateStyleRegistry::FindSlateStyle(PreviousAppStyleName);
	if (BaseStyle == nullptr)
	{
		return;
	}

	// Restore every installed slot in place first. Existing Slate widgets retain
	// pointers to these objects, so replacing or destroying the child style while
	// the editor is running would either preserve stale Light colors or dangle.
	if (Settings.bFixComboBoxRows)
	{
		GetMutableWidgetStyle<FTableRowStyle>(*Style, TEXT("ComboBox.Row")) =
			BaseStyle->GetWidgetStyle<FTableRowStyle>(TEXT("ComboBox.Row"));
	}

	if (Settings.bFixFocusedInputText)
	{
		RestoreWidgetStyles<FSpinBoxStyle>(*Style, *BaseStyle, FocusedSpinBoxStyleNames);
		RestoreWidgetStyles<FEditableTextBoxStyle>(*Style, *BaseStyle, FocusedEditableTextBoxStyleNames);
		RestoreWidgetStyles<FSearchBoxStyle>(*Style, *BaseStyle, FocusedSearchBoxStyleNames);
		RestoreWidgetStyles<FInlineEditableTextBlockStyle>(*Style, *BaseStyle, FocusedInlineTextStyleNames);
	}

	if (Settings.bFixDetailsPanelText)
	{
		GetMutableWidgetStyle<FInlineEditableTextBlockStyle>(
			*Style,
			TEXT("DetailsView.NameTextBlockStyle")) =
			BaseStyle->GetWidgetStyle<FInlineEditableTextBlockStyle>(TEXT("DetailsView.NameTextBlockStyle"));
		GetMutableWidgetStyle<FTextBlockStyle>(
			*Style,
			TEXT("DetailsView.ConstantTextBlockStyle")) =
			BaseStyle->GetWidgetStyle<FTextBlockStyle>(TEXT("DetailsView.ConstantTextBlockStyle"));
	}

	if (Settings.bFixBlueprintGraph)
	{
		GetMutableBrush(*Style, TEXT("Graph.Panel.SolidBackground")) =
			*BaseStyle->GetBrush(TEXT("Graph.Panel.SolidBackground"));
		GetMutableBrush(*Style, TEXT("Graph.Node.Body")) =
			*BaseStyle->GetBrush(TEXT("Graph.Node.Body"));
		GetMutableBrush(*Style, TEXT("Graph.Node.TintedBody")) =
			*BaseStyle->GetBrush(TEXT("Graph.Node.TintedBody"));
		GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("Graph.Node.NodeTitle")) =
			BaseStyle->GetWidgetStyle<FTextBlockStyle>(TEXT("Graph.Node.NodeTitle"));
		GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("Graph.Node.NodeTitleExtraLines")) =
			BaseStyle->GetWidgetStyle<FTextBlockStyle>(TEXT("Graph.Node.NodeTitleExtraLines"));
		GetMutableWidgetStyle<FInlineEditableTextBlockStyle>(
			*Style,
			TEXT("Graph.Node.NodeTitleInlineEditableText")) =
			BaseStyle->GetWidgetStyle<FInlineEditableTextBlockStyle>(
				TEXT("Graph.Node.NodeTitleInlineEditableText"));
		GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("GraphBreadcrumbButtonText")) =
			BaseStyle->GetWidgetStyle<FTextBlockStyle>(TEXT("GraphBreadcrumbButtonText"));
	}

	if (Settings.bFixDockTabs)
	{
		GetMutableWidgetStyle<FDockTabStyle>(*Style, TEXT("Docking.Tab")) =
			BaseStyle->GetWidgetStyle<FDockTabStyle>(TEXT("Docking.Tab"));
		GetMutableWidgetStyle<FDockTabStyle>(*Style, TEXT("Docking.MajorTab")) =
			BaseStyle->GetWidgetStyle<FDockTabStyle>(TEXT("Docking.MajorTab"));
	}

	if (Settings.bFixContentBrowserTiles)
	{
		static const FName BrushNames[] = {
			TEXT("ContentBrowser.AssetTileItem.AssetContentHoverBackground"),
			TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedBackground"),
			TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedHoverBackground"),
			TEXT("ContentBrowser.AssetTileItem.NameAreaHoverBackground"),
			TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedBackground"),
			TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedHoverBackground"),
			TEXT("ContentBrowser.AssetTileItem.FolderAreaHoveredBackground"),
			TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedBackground"),
			TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedHoverBackground")
		};
		for (const FName BrushName : BrushNames)
		{
			GetMutableBrush(*Style, BrushName) = *BaseStyle->GetBrush(BrushName);
		}
	}

	if (Settings.bFixAnimationTimeline)
	{
		GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("AnimTimeline.Outliner.Label")) =
			BaseStyle->GetWidgetStyle<FTextBlockStyle>(TEXT("AnimTimeline.Outliner.Label"));
		GetMutableColor(*Style, TEXT("AnimTimeline.Outliner.ItemColor")) =
			BaseStyle->GetColor(TEXT("AnimTimeline.Outliner.ItemColor"));
		GetMutableColor(*Style, TEXT("AnimTimeline.Outliner.HeaderColor")) =
			BaseStyle->GetColor(TEXT("AnimTimeline.Outliner.HeaderColor"));
	}

	if (bEnableLightThemeFixes)
	{
		if (Settings.bFixComboBoxRows)
		{
			GetMutableWidgetStyle<FTableRowStyle>(*Style, TEXT("ComboBox.Row"))
				.SetTextColor(FStyleColors::Foreground)
				.SetSelectedTextColor(FStyleColors::Foreground);
		}

		if (Settings.bFixFocusedInputText)
		{
			ApplyFocusedInputTextColors(
				*Style,
				Settings,
				FocusedSpinBoxStyleNames,
				FocusedEditableTextBoxStyleNames,
				FocusedSearchBoxStyleNames,
				FocusedInlineTextStyleNames);
		}

		if (Settings.bFixDetailsPanelText)
		{
			FInlineEditableTextBlockStyle& ObjectNameStyle =
				GetMutableWidgetStyle<FInlineEditableTextBlockStyle>(
					*Style,
					TEXT("DetailsView.NameTextBlockStyle"));
			ObjectNameStyle.TextStyle.SetColorAndOpacity(FStyleColors::Foreground);
			ObjectNameStyle.EditableTextBoxStyle
				.SetForegroundColor(FStyleColors::Foreground)
				.SetFocusedForegroundColor(FStyleColors::Foreground);
			GetMutableWidgetStyle<FTextBlockStyle>(
				*Style,
				TEXT("DetailsView.ConstantTextBlockStyle"))
				.SetColorAndOpacity(FStyleColors::Foreground);
		}

		if (Settings.bFixBlueprintGraph)
		{
			GetMutableBrush(*Style, TEXT("Graph.Panel.SolidBackground")) =
				FSlateColorBrush(FStyleColors::Recessed);
			GetMutableBrush(*Style, TEXT("Graph.Node.Body")) =
				FSlateRoundedBoxBrush(
					Settings.NodeBodyColor,
					Settings.NodeCornerRadius,
					Settings.NodeOutlineColor,
					Settings.NodeOutlineWidth);
			GetMutableBrush(*Style, TEXT("Graph.Node.TintedBody")) =
				FSlateRoundedBoxBrush(
					Settings.NodeTintedBodyColor,
					Settings.NodeCornerRadius,
					Settings.NodeOutlineColor,
					Settings.NodeOutlineWidth);
			GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("Graph.Node.NodeTitle"))
				.SetColorAndOpacity(Settings.NodeTitleTextColor);
			GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("Graph.Node.NodeTitleExtraLines"))
				.SetColorAndOpacity(Settings.NodeSubtitleTextColor);
			GetMutableWidgetStyle<FInlineEditableTextBlockStyle>(
				*Style,
				TEXT("Graph.Node.NodeTitleInlineEditableText"))
				.SetTextStyle(GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("Graph.Node.NodeTitle")));
			GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("GraphBreadcrumbButtonText"))
				.SetColorAndOpacity(Settings.BreadcrumbTextColor);
		}

		if (Settings.bFixDockTabs)
		{
			SetLightTabForegrounds(GetMutableWidgetStyle<FDockTabStyle>(*Style, TEXT("Docking.Tab")));
			SetLightTabForegrounds(GetMutableWidgetStyle<FDockTabStyle>(*Style, TEXT("Docking.MajorTab")));
		}

		if (Settings.bFixContentBrowserTiles)
		{
			const float Radius = Settings.AssetTileCornerRadius;
			const FVector4 NameAreaRadius(0.0f, 0.0f, Radius, Radius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.AssetContentHoverBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, FVector4(Radius));
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, FVector4(Radius));
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedHoverBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, FVector4(Radius));
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.NameAreaHoverBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, NameAreaRadius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, NameAreaRadius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedHoverBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, NameAreaRadius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.FolderAreaHoveredBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileHoverColor, Radius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedColor, Radius);
			GetMutableBrush(*Style, TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedHoverBackground")) =
				FSlateRoundedBoxBrush(Settings.AssetTileSelectedHoverColor, Radius);
		}

		if (Settings.bFixAnimationTimeline)
		{
			GetMutableWidgetStyle<FTextBlockStyle>(*Style, TEXT("AnimTimeline.Outliner.Label"))
				.SetColorAndOpacity(Settings.AnimationTimelineLabelColor)
				.SetShadowOffset(FVector2f::ZeroVector)
				.SetShadowColorAndOpacity(FLinearColor::Transparent);
			GetMutableColor(*Style, TEXT("AnimTimeline.Outliner.ItemColor")) =
				Settings.AnimationTimelineItemColor;
			GetMutableColor(*Style, TEXT("AnimTimeline.Outliner.HeaderColor")) =
				Settings.AnimationTimelineHeaderColor;
		}

		FAppStyle::SetAppStyleSet(*Style);
	}
	else if (FAppStyle::GetAppStyleSetName() == Style->GetStyleSetName())
	{
		FAppStyle::SetAppStyleSetName(PreviousAppStyleName);
	}

	if (FSlateApplicationBase::IsInitialized())
	{
		FSlateApplicationBase::Get().InvalidateAllWidgets(false);
	}
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
	FocusedSpinBoxStyleNames.Reset();
	FocusedEditableTextBoxStyleNames.Reset();
	FocusedSearchBoxStyleNames.Reset();
	FocusedInlineTextStyleNames.Reset();
}
