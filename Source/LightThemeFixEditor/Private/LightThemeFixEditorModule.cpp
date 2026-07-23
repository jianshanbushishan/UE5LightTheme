#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "ActorTreeItem.h"
#include "GraphEditorSettings.h"
#include "HAL/FileManager.h"
#include "ISceneOutlinerColumn.h"
#include "Interfaces/IPluginManager.h"
#include "Layout/WidgetPath.h"
#include "LightThemeFixSettings.h"
#include "LightThemeFixStyle.h"
#include "Misc/CoreMisc.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "SceneOutlinerModule.h"
#include "SceneOutlinerPublicTypes.h"
#include "Settings/EditorStyleSettings.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/SWindow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMisc.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogLightThemeFix, Log, All);

namespace
{
	constexpr TCHAR PluginName[] = TEXT("LightThemeFix");
	constexpr TCHAR BundledThemeRelativePath[] = TEXT("Resources/Themes/Light.json");
	constexpr TCHAR InstalledThemeFileName[] = TEXT("Light.json");
	constexpr TCHAR WindowsThemeRegistryStore[] = TEXT("Microsoft");
	constexpr TCHAR WindowsThemeRegistrySection[] = TEXT("Windows\\CurrentVersion\\Themes\\Personalize");
	constexpr TCHAR WindowsAppsUseLightThemeValue[] = TEXT("AppsUseLightTheme");

	const FGuid BundledLightThemeId(0xA6C86E0F, 0x42D14AA5, 0x9446D15D, 0x57244A78);
	const FGuid DefaultDarkThemeId(0x13438026, 0x5FBB4A9C, 0xA00A1DC9, 0x770217B8);

	const FName EditableTextType(TEXT("SEditableText"));
	const FName EditableTextBoxType(TEXT("SEditableTextBox"));
	const FName SearchBoxType(TEXT("SSearchBox"));
	const FName FilterSearchBoxType(TEXT("SFilterSearchBoxImpl"));
	const FName MultiLineEditableTextBoxType(TEXT("SMultiLineEditableTextBox"));
	const FName TextBlockType(TEXT("STextBlock"));
	const FName SlowTaskWidgetType(TEXT("SSlowTaskWidget"));
	const FName ImageType(TEXT("SImage"));
	const FName LiveCodingProgressDialogBackgroundStyleName(
		TEXT("LightThemeFix.LiveCoding.ProgressDialogBackground"));
	const FLinearColor EnginePIEActorLabelColor(0.9f, 0.8f, 0.4f);

	bool ShouldApplyLightThemeFixes(const ULightThemeFixSettings& Settings)
	{
		if (!Settings.bApplyOnlyToLightThemes)
		{
			return true;
		}

		const FLinearColor ThemeBackground = USlateThemeManager::Get().GetColor(EStyleColor::Background);
		return ThemeBackground.GetLuminance() > Settings.LightThemeLuminanceThreshold;
	}

	bool IsEnginePIEActorLabelColor(const FSlateColor& Color)
	{
		return Color.IsColorSpecified() &&
			Color.GetSpecifiedColor().Equals(EnginePIEActorLabelColor);
	}

	FSlateColor ResolvePIEActorLabelColor(
		const TWeakPtr<SWidget>& WeakActorTreeLabel,
		const TWeakPtr<ISceneOutlinerTreeItem>& WeakTreeItem)
	{
		const TSharedPtr<SWidget> ActorTreeLabel = WeakActorTreeLabel.Pin();
		const TSharedPtr<ISceneOutlinerTreeItem> TreeItem = WeakTreeItem.Pin();
		if (!ActorTreeLabel.IsValid() || !TreeItem.IsValid())
		{
			return FSlateColor::UseForeground();
		}

		const FActorTreeItem* ActorItem = TreeItem->CastTo<FActorTreeItem>();
		if (ActorItem == nullptr || ActorItem->bExistsInCurrentWorldAndPIE)
		{
			return FSlateColor::UseForeground();
		}

		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		if (!Settings.bFixPIEActorLabels || !ShouldApplyLightThemeFixes(Settings))
		{
			return FSlateColor::UseForeground();
		}

		// Query the untouched SActorTreeLabel root so UE retains precedence for
		// disabled rows, invalid actors, and invalid drag/drop targets. Its pale
		// yellow is returned only for actors exclusive to the active PIE world.
		if (!IsEnginePIEActorLabelColor(ActorTreeLabel->GetForegroundColor()))
		{
			return FSlateColor::UseForeground();
		}

		return FSlateColor(Settings.PIEActorLabelColor);
	}

	void BindPIEActorLabelDescendantColors(
		const TSharedRef<SWidget>& Widget,
		const TWeakPtr<SWidget>& WeakActorTreeLabel,
		const TWeakPtr<ISceneOutlinerTreeItem>& WeakTreeItem)
	{
		const TAttribute<FSlateColor> DynamicColor = TAttribute<FSlateColor>::CreateLambda(
			[WeakActorTreeLabel, WeakTreeItem]()
			{
				return ResolvePIEActorLabelColor(WeakActorTreeLabel, WeakTreeItem);
			});

		const FName WidgetType = Widget->GetType();
		if (WidgetType == TextBlockType)
		{
			StaticCastSharedRef<STextBlock>(Widget)->SetColorAndOpacity(DynamicColor);
		}
		else if (WidgetType == ImageType)
		{
			StaticCastSharedRef<SImage>(Widget)->SetColorAndOpacity(DynamicColor);
		}

		if (FChildren* Children = Widget->GetChildren())
		{
			for (int32 ChildIndex = 0; ChildIndex < Children->Num(); ++ChildIndex)
			{
				BindPIEActorLabelDescendantColors(
					Children->GetChildAt(ChildIndex),
					WeakActorTreeLabel,
					WeakTreeItem);
			}
		}
	}

	class FLightThemeFixActorLabelColumn final : public ISceneOutlinerColumn
	{
	public:
		explicit FLightThemeFixActorLabelColumn(TSharedRef<ISceneOutlinerColumn> InInnerColumn)
			: InnerColumn(MoveTemp(InInnerColumn))
		{
		}

		virtual FName GetColumnID() override
		{
			return InnerColumn->GetColumnID();
		}

		virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override
		{
			return InnerColumn->ConstructHeaderRowColumn();
		}

		virtual const TSharedRef<SWidget> ConstructRowWidget(
			FSceneOutlinerTreeItemRef TreeItem,
			const STableRow<FSceneOutlinerTreeItemPtr>& Row) override
		{
			const TSharedRef<SWidget> LabelWidget = InnerColumn->ConstructRowWidget(TreeItem, Row);
			if (TreeItem->CastTo<FActorTreeItem>() != nullptr)
			{
				BindPIEActorLabelDescendantColors(LabelWidget, LabelWidget, TreeItem);
				if (!bLoggedActorRowBinding)
				{
					UE_LOG(LogLightThemeFix, Log, TEXT("Bound dynamic contrast attributes to Actor Browser label rows."));
					bLoggedActorRowBinding = true;
				}
			}
			return LabelWidget;
		}

		virtual void Tick(double InCurrentTime, float InDeltaTime) override
		{
			InnerColumn->Tick(InCurrentTime, InDeltaTime);
		}

		virtual void PopulateSearchStrings(
			const ISceneOutlinerTreeItem& Item,
			TArray<FString>& OutSearchStrings) const override
		{
			InnerColumn->PopulateSearchStrings(Item, OutSearchStrings);
		}

		virtual bool SupportsSorting() const override
		{
			return InnerColumn->SupportsSorting();
		}

		virtual void SortItems(
			TArray<FSceneOutlinerTreeItemPtr>& OutItems,
			const EColumnSortMode::Type SortMode) const override
		{
			InnerColumn->SortItems(OutItems, SortMode);
		}

		virtual void OnSortRequested(
			const EColumnSortPriority::Type SortPriority,
			const EColumnSortMode::Type InSortMode) override
		{
			InnerColumn->OnSortRequested(SortPriority, InSortMode);
		}

		virtual bool IsSortReady() override
		{
			return InnerColumn->IsSortReady();
		}

	private:
		TSharedRef<ISceneOutlinerColumn> InnerColumn;
		bool bLoggedActorRowBinding = false;
	};

	FGuid FindThemeId(
		const USlateThemeManager& ThemeManager,
		const FGuid& PreferredThemeId,
		const TCHAR* DisplayName)
	{
		if (ThemeManager.DoesThemeExist(PreferredThemeId))
		{
			return PreferredThemeId;
		}

		for (const FStyleTheme& Theme : ThemeManager.GetThemes())
		{
			if (Theme.DisplayName.ToString().Equals(DisplayName, ESearchCase::IgnoreCase))
			{
				return Theme.Id;
			}
		}

		return FGuid();
	}

	bool IsSingleLineTextBox(const FName WidgetType)
	{
		// SNew records the concrete Slate class name. Derived search controls do
		// not report SEditableTextBox even though they can safely be cast to it.
		return WidgetType == EditableTextBoxType ||
			WidgetType == SearchBoxType ||
			WidgetType == FilterSearchBoxType;
	}
}

class FLightThemeFixEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();

		InstallBundledTheme(Settings);
		ApplyWindowsTheme(Settings);
		SaveOriginalAnimationTimelineColors(Settings);
		Style.Initialize(Settings);
		Style.ApplyTheme(Settings, ShouldApplyToCurrentTheme(Settings));
		SaveOriginalGraphColors();
		RegisterSceneOutlinerColumnOverride();

		FCoreDelegates::PreSlateModalWithContext.AddRaw(this, &FLightThemeFixEditorModule::HandlePreSlateModal);
		bPreSlateModalDelegateRegistered = true;

		USlateThemeManager::Get().OnThemeChanged().AddRaw(this, &FLightThemeFixEditorModule::HandleThemeChanged);
		bThemeDelegateRegistered = true;
		if (Settings.bFixBlueprintGraph)
		{
			ApplyGraphColors();
		}

		if (Settings.bFixTooltipText || Settings.bFixFocusedInputText || Settings.bFixLiveCodingDialog)
		{
			DynamicContrastTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FLightThemeFixEditorModule::UpdateDynamicContrast),
				Settings.TooltipRefreshIntervalSeconds);
		}

		if (Settings.bFollowWindowsTheme)
		{
			WindowsThemeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FLightThemeFixEditorModule::UpdateWindowsTheme),
				Settings.WindowsThemeCheckIntervalSeconds);
		}
	}

	virtual void ShutdownModule() override
	{
		// CoreUObject and parts of Slate may already be gone when editor modules
		// unload during process exit. Do not touch settings or global managers.
		if (IsEngineExitRequested())
		{
			return;
		}

		if (bThemeDelegateRegistered)
		{
			USlateThemeManager::Get().OnThemeChanged().RemoveAll(this);
			bThemeDelegateRegistered = false;
		}

		if (bPreSlateModalDelegateRegistered)
		{
			FCoreDelegates::PreSlateModalWithContext.RemoveAll(this);
			bPreSlateModalDelegateRegistered = false;
		}

		if (SceneOutlinerColumnsDelegateHandle.IsValid() &&
			FModuleManager::Get().IsModuleLoaded("SceneOutliner"))
		{
			FModuleManager::GetModuleChecked<FSceneOutlinerModule>("SceneOutliner")
				.OnCreateActorBrowserColumns().Remove(SceneOutlinerColumnsDelegateHandle);
			SceneOutlinerColumnsDelegateHandle.Reset();
		}

		if (DynamicContrastTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(DynamicContrastTickerHandle);
			DynamicContrastTickerHandle.Reset();
		}

		if (WindowsThemeTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(WindowsThemeTickerHandle);
			WindowsThemeTickerHandle.Reset();
		}

		RestoreGraphColors();
		Style.Shutdown();
	}

private:
	void RegisterSceneOutlinerColumnOverride()
	{
		FSceneOutlinerModule& SceneOutlinerModule =
			FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");
		SceneOutlinerColumnsDelegateHandle = SceneOutlinerModule.OnCreateActorBrowserColumns().AddRaw(
			this,
			&FLightThemeFixEditorModule::HandleCreateActorBrowserColumns);
		UE_LOG(LogLightThemeFix, Log, TEXT("Registered the Actor Browser label-column contrast override."));
	}

	void HandleCreateActorBrowserColumns(FSceneOutlinerInitializationOptions& InitOptions, UWorld*)
	{
		FSceneOutlinerColumnInfo* LabelColumnInfo =
			InitOptions.ColumnMap.Find(FSceneOutlinerBuiltInColumnTypes::Label());
		if (LabelColumnInfo == nullptr)
		{
			UE_LOG(LogLightThemeFix, Warning, TEXT("Actor Browser has no Label column to decorate."));
			return;
		}

		const FCreateSceneOutlinerColumn OriginalFactory = LabelColumnInfo->Factory;
		LabelColumnInfo->Factory = FCreateSceneOutlinerColumn::CreateLambda(
			[OriginalFactory](ISceneOutliner& Outliner) -> TSharedRef<ISceneOutlinerColumn>
			{
				TSharedPtr<ISceneOutlinerColumn> OriginalColumn;
				if (OriginalFactory.IsBound())
				{
					OriginalColumn = OriginalFactory.Execute(Outliner);
				}
				else
				{
					OriginalColumn = FModuleManager::GetModuleChecked<FSceneOutlinerModule>("SceneOutliner")
						.FactoryColumn(FSceneOutlinerBuiltInColumnTypes::Label(), Outliner);
				}

				checkf(OriginalColumn.IsValid(), TEXT("Scene Outliner Label column factory is unavailable."));
				return MakeShared<FLightThemeFixActorLabelColumn>(OriginalColumn.ToSharedRef());
			});

		UE_LOG(LogLightThemeFix, Log, TEXT("Decorated an Actor Browser Label column."));
	}

	static void InstallBundledTheme(const ULightThemeFixSettings& Settings)
	{
		if (!Settings.bInstallBundledTheme)
		{
			return;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
		if (!Plugin.IsValid())
		{
			UE_LOG(LogLightThemeFix, Warning, TEXT("Could not find the LightThemeFix plugin descriptor."));
			return;
		}

		const FString SourcePath = Plugin->GetBaseDir() / BundledThemeRelativePath;
		const FString TargetDirectory = USlateThemeManager::Get().GetUserThemeDir();
		const FString TargetPath = TargetDirectory / InstalledThemeFileName;
		if (!IFileManager::Get().FileExists(*SourcePath))
		{
			UE_LOG(LogLightThemeFix, Warning, TEXT("Bundled theme is missing: %s"), *SourcePath);
			return;
		}

		if (IFileManager::Get().FileExists(*TargetPath))
		{
			return;
		}

		IFileManager::Get().MakeDirectory(*TargetDirectory, true);
		if (IFileManager::Get().Copy(*TargetPath, *SourcePath, true, true) == COPY_OK)
		{
			USlateThemeManager::Get().LoadThemes();
			UE_LOG(LogLightThemeFix, Log, TEXT("Installed bundled theme to %s"), *TargetPath);
		}
		else
		{
			UE_LOG(LogLightThemeFix, Warning, TEXT("Could not install bundled theme from %s"), *SourcePath);
		}
	}

	bool UpdateWindowsTheme(float)
	{
		if (IsEngineExitRequested())
		{
			return true;
		}

		ApplyWindowsTheme(*GetDefault<ULightThemeFixSettings>());
		return true;
	}

	void ApplyWindowsTheme(const ULightThemeFixSettings& Settings)
	{
		if (!Settings.bFollowWindowsTheme)
		{
			return;
		}

#if PLATFORM_WINDOWS
		FString RegistryValue;
		if (!FWindowsPlatformMisc::GetStoredValue(
			WindowsThemeRegistryStore,
			WindowsThemeRegistrySection,
			WindowsAppsUseLightThemeValue,
			RegistryValue))
		{
			if (!bWindowsThemeReadFailureLogged)
			{
				UE_LOG(LogLightThemeFix, Warning, TEXT("Could not read the Windows app theme; automatic theme switching is paused."));
				bWindowsThemeReadFailureLogged = true;
			}
			return;
		}

		bWindowsThemeReadFailureLogged = false;
		const bool bUseLightTheme = FCString::Atoi(*RegistryValue) != 0;
		USlateThemeManager& ThemeManager = USlateThemeManager::Get();
		const TCHAR* DesiredThemeName = bUseLightTheme ? TEXT("Light") : TEXT("Dark");
		const FGuid DesiredThemeId = FindThemeId(
			ThemeManager,
			bUseLightTheme ? BundledLightThemeId : DefaultDarkThemeId,
			DesiredThemeName);
		if (!DesiredThemeId.IsValid())
		{
			if (!bMissingThemeWarningLogged)
			{
				UE_LOG(
					LogLightThemeFix,
					Warning,
					TEXT("Could not switch to the %s theme because it is not available."),
					DesiredThemeName);
				bMissingThemeWarningLogged = true;
			}
			return;
		}

		if (ThemeManager.GetCurrentThemeID() == DesiredThemeId)
		{
			return;
		}

		bMissingThemeWarningLogged = false;
		ThemeManager.ApplyTheme(DesiredThemeId);

		UEditorStyleSettings* EditorStyleSettings = GetMutableDefault<UEditorStyleSettings>();
		EditorStyleSettings->CurrentAppliedTheme = DesiredThemeId;
		EditorStyleSettings->SaveConfig();

		UE_LOG(
			LogLightThemeFix,
			Log,
			TEXT("Switched Unreal Editor to the %s theme to match Windows."),
			DesiredThemeName);
#endif
	}

	void SaveOriginalGraphColors()
	{
		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		if (!Settings.bFixBlueprintGraph)
		{
			return;
		}

		const UEditorStyleSettings* EditorStyleSettings = GetDefault<UEditorStyleSettings>();
		OriginalRegularColor = EditorStyleSettings->RegularColor;
		OriginalRuleColor = EditorStyleSettings->RuleColor;
		OriginalCenterColor = EditorStyleSettings->CenterColor;
		OriginalExecutionPinTypeColor = GetDefault<UGraphEditorSettings>()->ExecutionPinTypeColor;
		bHasSavedGraphColors = true;
	}

	void HandleThemeChanged(FGuid)
	{
		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		const bool bApplyLightThemeFixes = ShouldApplyToCurrentTheme(Settings);
		Style.ApplyTheme(Settings, bApplyLightThemeFixes);
		ApplyGraphColors();
		RefreshAnimationTimelineColors(Settings, bApplyLightThemeFixes);
		UpdateDynamicContrast(0.0f);
		UE_LOG(
			LogLightThemeFix,
			Log,
			TEXT("%s Light-theme-only contrast overrides after the editor theme changed."),
			bApplyLightThemeFixes ? TEXT("Enabled") : TEXT("Disabled"));
	}

	void SaveOriginalAnimationTimelineColors(const ULightThemeFixSettings& Settings)
	{
		if (!Settings.bFixAnimationTimeline)
		{
			return;
		}

		OriginalAnimationTimelineItemColor =
			FAppStyle::GetColor(TEXT("AnimTimeline.Outliner.ItemColor"));
		OriginalAnimationTimelineHeaderColor =
			FAppStyle::GetColor(TEXT("AnimTimeline.Outliner.HeaderColor"));
		bHasSavedAnimationTimelineColors = true;
	}

	void RefreshAnimationTimelineColors(
		const ULightThemeFixSettings& Settings,
		const bool bApplyLightThemeFixes) const
	{
		if (!bHasSavedAnimationTimelineColors || !FSlateApplication::IsInitialized())
		{
			return;
		}

		TArray<TSharedRef<SWindow>> VisibleWindows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(VisibleWindows);
		for (const TSharedRef<SWindow>& Window : VisibleWindows)
		{
			RefreshAnimationTimelineWidgetColors(
				Window,
				Settings,
				bApplyLightThemeFixes,
				false);
		}
	}

	void RefreshAnimationTimelineWidgetColors(
		const TSharedRef<SWidget>& Widget,
		const ULightThemeFixSettings& Settings,
		const bool bApplyLightThemeFixes,
		const bool bInsideAnimationTimeline) const
	{
		const bool bIsInsideAnimationTimeline =
			bInsideAnimationTimeline || Widget->GetType() == FName(TEXT("SAnimTimeline"));
		if (bIsInsideAnimationTimeline && Widget->GetType() == FName(TEXT("SBorder")))
		{
			const TSharedRef<SBorder> Border = StaticCastSharedRef<SBorder>(Widget);
			const FSlateColor CurrentColor = Border->GetBorderBackgroundColor();
			if (CurrentColor.IsColorSpecified())
			{
				const FLinearColor SpecifiedColor = CurrentColor.GetSpecifiedColor();
				const FLinearColor SourceItemColor = bApplyLightThemeFixes
					? OriginalAnimationTimelineItemColor
					: Settings.AnimationTimelineItemColor;
				const FLinearColor SourceHeaderColor = bApplyLightThemeFixes
					? OriginalAnimationTimelineHeaderColor
					: Settings.AnimationTimelineHeaderColor;
				if (SpecifiedColor.Equals(SourceItemColor))
				{
					Border->SetBorderBackgroundColor(
						bApplyLightThemeFixes
							? Settings.AnimationTimelineItemColor
							: OriginalAnimationTimelineItemColor);
					Border->Invalidate(EInvalidateWidgetReason::Paint);
				}
				else if (SpecifiedColor.Equals(SourceHeaderColor))
				{
					Border->SetBorderBackgroundColor(
						bApplyLightThemeFixes
							? Settings.AnimationTimelineHeaderColor
							: OriginalAnimationTimelineHeaderColor);
					Border->Invalidate(EInvalidateWidgetReason::Paint);
				}
			}
		}

		if (FChildren* Children = Widget->GetChildren())
		{
			for (int32 ChildIndex = 0; ChildIndex < Children->Num(); ++ChildIndex)
			{
				RefreshAnimationTimelineWidgetColors(
					Children->GetChildAt(ChildIndex),
					Settings,
					bApplyLightThemeFixes,
					bIsInsideAnimationTimeline);
			}
		}
	}

	void HandlePreSlateModal(const FCoreDelegates::FModalWindowContext& Context) const
	{
		if (IsEngineExitRequested() || !Context.bIsSlowTaskWindow.Get(false))
		{
			return;
		}

		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		if (!Settings.bFixLiveCodingDialog)
		{
			return;
		}

		// In UE 5.8, FSlateApplication passes the address of the SWindow through
		// WindowIdentifier immediately after its content has been assigned and
		// before the slow-task window is first drawn.
		SWindow* const ModalWindow = reinterpret_cast<SWindow*>(Context.WindowIdentifier);
		if (ModalWindow != nullptr)
		{
			ApplyLiveCodingDialogStyle(
				ModalWindow->GetContent(),
				Settings,
				ShouldApplyToCurrentTheme(Settings));
		}
	}

	bool UpdateDynamicContrast(float) const
	{
		// The ticker can run while editor shutdown is already in progress. Keep
		// this guard ahead of settings and global Slate/theme manager access.
		if (IsEngineExitRequested() || !FSlateApplication::IsInitialized())
		{
			return true;
		}

		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		const bool bApplyLightThemeFixes = ShouldApplyToCurrentTheme(Settings);

		if (Settings.bFixTooltipText)
		{
			TArray<TSharedRef<SWindow>> VisibleWindows;
			FSlateApplication::Get().GetAllVisibleWindowsOrdered(VisibleWindows);
			for (const TSharedRef<SWindow>& Window : VisibleWindows)
			{
				if (Window->GetType() == EWindowType::ToolTip)
				{
					ApplyTooltipTextColor(
						Window,
						bApplyLightThemeFixes
							? FSlateColor(Settings.TooltipTextColor)
							: FSlateColor::UseForeground());
				}
			}
		}

		if (Settings.bFixFocusedInputText)
		{
			ApplyFocusedInputTextColor(Settings, bApplyLightThemeFixes);
		}

		if (Settings.bFixLiveCodingDialog)
		{
			TArray<TSharedRef<SWindow>> VisibleWindows;
			FSlateApplication::Get().GetAllVisibleWindowsOrdered(VisibleWindows);
			for (const TSharedRef<SWindow>& Window : VisibleWindows)
			{
				ApplyLiveCodingDialogStyle(Window, Settings, bApplyLightThemeFixes);
			}
		}

		return true;
	}

	static bool HasLightBorderBackground(const SBorder& Border, const float LuminanceThreshold)
	{
		const FSlateBrush* BorderBrush = Border.GetBorderImage();
		if (BorderBrush == nullptr || !BorderBrush->IsSet())
		{
			return false;
		}

		const FWidgetStyle WidgetStyle;
		const FLinearColor BackgroundColor =
			BorderBrush->GetTint(WidgetStyle) * Border.GetBorderBackgroundColor().GetColor(WidgetStyle);
		return BackgroundColor.GetLuminance() > LuminanceThreshold;
	}

	static void ApplyFocusedInputTextColor(
		const ULightThemeFixSettings& Settings,
		const bool bApplyLightThemeFixes)
	{
		const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
		if (!FocusedWidget.IsValid())
		{
			return;
		}

		FWidgetPath FocusPath;
		if (!FSlateApplication::Get().GeneratePathToWidgetUnchecked(FocusedWidget.ToSharedRef(), FocusPath))
		{
			return;
		}

		for (int32 WidgetIndex = 0; WidgetIndex < FocusPath.Widgets.Num(); ++WidgetIndex)
		{
			const TSharedRef<SWidget>& PathWidget = FocusPath.Widgets[WidgetIndex].Widget;
			const FName WidgetType = PathWidget->GetType();
			if (IsSingleLineTextBox(WidgetType))
			{
				const TSharedRef<SEditableTextBox> TextBox = StaticCastSharedRef<SEditableTextBox>(PathWidget);
				if (bApplyLightThemeFixes &&
					HasLightBorderBackground(*TextBox, Settings.LightThemeLuminanceThreshold))
				{
					TextBox->SetFocusedForegroundColor(Settings.FocusedInputTextColor);
				}
				else if (!bApplyLightThemeFixes)
				{
					TextBox->SetFocusedForegroundColor(FSlateColor::UseForeground());
				}
				TextBox->Invalidate(EInvalidateWidgetReason::Paint);

				// Remove a direct leaf override so the editable text inherits the
				// foreground from its text box in both Light and Dark themes.
				if (FocusedWidget->GetType() == EditableTextType)
				{
					StaticCastSharedPtr<SEditableText>(FocusedWidget)->SetColorAndOpacity(
						FSlateColor::UseForeground());
				}
				return;
			}

			if (WidgetType == MultiLineEditableTextBoxType)
			{
				const TSharedRef<SMultiLineEditableTextBox> TextBox =
					StaticCastSharedRef<SMultiLineEditableTextBox>(PathWidget);
				if (bApplyLightThemeFixes &&
					HasLightBorderBackground(*TextBox, Settings.LightThemeLuminanceThreshold))
				{
					TextBox->SetTextBoxForegroundColor(Settings.FocusedInputTextColor);
				}
				else if (!bApplyLightThemeFixes)
				{
					TextBox->SetTextBoxForegroundColor(FSlateColor::UseForeground());
				}
				TextBox->Invalidate(EInvalidateWidgetReason::Paint);
				return;
			}
		}
	}

	static void ApplyTooltipTextColor(const TSharedRef<SWidget>& Widget, const FSlateColor& TextColor)
	{
		if (Widget->GetType() == TextBlockType)
		{
			StaticCastSharedRef<STextBlock>(Widget)->SetColorAndOpacity(TextColor);
		}

		if (FChildren* Children = Widget->GetChildren())
		{
			for (int32 ChildIndex = 0; ChildIndex < Children->Num(); ++ChildIndex)
			{
				ApplyTooltipTextColor(Children->GetChildAt(ChildIndex), TextColor);
			}
		}
	}

	static void ApplyLiveCodingDialogStyle(
		const TSharedRef<SWidget>& Widget,
		const ULightThemeFixSettings& Settings,
		const bool bUseLightDialogStyle)
	{
		if (Widget->GetType() == SlowTaskWidgetType)
		{
			const TSharedRef<SBorder> Dialog = StaticCastSharedRef<SBorder>(Widget);
			Dialog->SetBorderImage(
				FAppStyle::GetBrush(
					bUseLightDialogStyle
						? LiveCodingProgressDialogBackgroundStyleName
						: FName(TEXT("Brushes.Header"))));
			Dialog->SetForegroundColor(
				bUseLightDialogStyle ? FSlateColor(Settings.LiveCodingDialogTextColor) : FSlateColor::UseForeground());
			Dialog->Invalidate(EInvalidateWidgetReason::Paint);
			return;
		}

		if (FChildren* Children = Widget->GetChildren())
		{
			for (int32 ChildIndex = 0; ChildIndex < Children->Num(); ++ChildIndex)
			{
				ApplyLiveCodingDialogStyle(Children->GetChildAt(ChildIndex), Settings, bUseLightDialogStyle);
			}
		}
	}

	static bool ShouldApplyToCurrentTheme(const ULightThemeFixSettings& Settings)
	{
		return ShouldApplyLightThemeFixes(Settings);
	}

	void ApplyGraphColors() const
	{
		if (!bHasSavedGraphColors)
		{
			return;
		}

		const ULightThemeFixSettings& Settings = *GetDefault<ULightThemeFixSettings>();
		if (!ShouldApplyToCurrentTheme(Settings))
		{
			RestoreGraphColors();
			return;
		}

		UEditorStyleSettings* EditorStyleSettings = GetMutableDefault<UEditorStyleSettings>();
		EditorStyleSettings->RegularColor = Settings.GraphRegularGridColor;
		EditorStyleSettings->RuleColor = Settings.GraphRuleGridColor;
		EditorStyleSettings->CenterColor = Settings.GraphCenterGridColor;
		GetMutableDefault<UGraphEditorSettings>()->ExecutionPinTypeColor = Settings.ExecutionWireColor;
	}

	void RestoreGraphColors() const
	{
		if (!bHasSavedGraphColors)
		{
			return;
		}

		UEditorStyleSettings* EditorStyleSettings = GetMutableDefault<UEditorStyleSettings>();
		EditorStyleSettings->RegularColor = OriginalRegularColor;
		EditorStyleSettings->RuleColor = OriginalRuleColor;
		EditorStyleSettings->CenterColor = OriginalCenterColor;
		GetMutableDefault<UGraphEditorSettings>()->ExecutionPinTypeColor = OriginalExecutionPinTypeColor;
	}

	FLightThemeFixStyle Style;
	FLinearColor OriginalRegularColor;
	FLinearColor OriginalRuleColor;
	FLinearColor OriginalCenterColor;
	FLinearColor OriginalExecutionPinTypeColor;
	FLinearColor OriginalAnimationTimelineItemColor;
	FLinearColor OriginalAnimationTimelineHeaderColor;
	FTSTicker::FDelegateHandle DynamicContrastTickerHandle;
	FTSTicker::FDelegateHandle WindowsThemeTickerHandle;
	bool bThemeDelegateRegistered = false;
	bool bPreSlateModalDelegateRegistered = false;
	bool bHasSavedGraphColors = false;
	bool bHasSavedAnimationTimelineColors = false;
	bool bWindowsThemeReadFailureLogged = false;
	bool bMissingThemeWarningLogged = false;
	FDelegateHandle SceneOutlinerColumnsDelegateHandle;
};

IMPLEMENT_MODULE(FLightThemeFixEditorModule, LightThemeFixEditor)
