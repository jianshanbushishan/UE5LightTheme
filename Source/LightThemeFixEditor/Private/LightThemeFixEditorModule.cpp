#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "GraphEditorSettings.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Layout/WidgetPath.h"
#include "LightThemeFixSettings.h"
#include "LightThemeFixStyle.h"
#include "Misc/CoreMisc.h"
#include "Modules/ModuleManager.h"
#include "Settings/EditorStyleSettings.h"
#include "Styling/StyleColors.h"
#include "Widgets/SWindow.h"
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
		Style.Initialize(Settings);
		SaveOriginalGraphColors();

		if (Settings.bFixBlueprintGraph)
		{
			USlateThemeManager::Get().OnThemeChanged().AddRaw(this, &FLightThemeFixEditorModule::HandleThemeChanged);
			bThemeDelegateRegistered = true;
			ApplyGraphColors();
		}

		if (Settings.bFixTooltipText || Settings.bFixFocusedInputText)
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
		ApplyGraphColors();
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
		if (!ShouldApplyToCurrentTheme(Settings))
		{
			return true;
		}

		if (Settings.bFixTooltipText)
		{
			TArray<TSharedRef<SWindow>> VisibleWindows;
			FSlateApplication::Get().GetAllVisibleWindowsOrdered(VisibleWindows);
			for (const TSharedRef<SWindow>& Window : VisibleWindows)
			{
				if (Window->GetType() == EWindowType::ToolTip)
				{
					ApplyTooltipTextColor(Window, Settings.TooltipTextColor);
				}
			}
		}

		if (Settings.bFixFocusedInputText)
		{
			ApplyFocusedInputTextColor(Settings);
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

	static void ApplyFocusedInputTextColor(const ULightThemeFixSettings& Settings)
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
				if (HasLightBorderBackground(*TextBox, Settings.LightThemeLuminanceThreshold))
				{
					TextBox->SetFocusedForegroundColor(Settings.FocusedInputTextColor);
					TextBox->Invalidate(EInvalidateWidgetReason::Paint);

					// Remove a direct leaf override so the editable text inherits the corrected
					// foreground from its SEditableTextBox parent.
					if (FocusedWidget->GetType() == EditableTextType)
					{
						StaticCastSharedPtr<SEditableText>(FocusedWidget)->SetColorAndOpacity(
							FSlateColor::UseForeground());
					}
				}
				return;
			}

			if (WidgetType == MultiLineEditableTextBoxType)
			{
				const TSharedRef<SMultiLineEditableTextBox> TextBox =
					StaticCastSharedRef<SMultiLineEditableTextBox>(PathWidget);
				if (HasLightBorderBackground(*TextBox, Settings.LightThemeLuminanceThreshold))
				{
					TextBox->SetTextBoxForegroundColor(Settings.FocusedInputTextColor);
					TextBox->Invalidate(EInvalidateWidgetReason::Paint);
				}
				return;
			}
		}
	}

	static void ApplyTooltipTextColor(const TSharedRef<SWidget>& Widget, const FLinearColor& TextColor)
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

	static bool ShouldApplyToCurrentTheme(const ULightThemeFixSettings& Settings)
	{
		if (!Settings.bApplyOnlyToLightThemes)
		{
			return true;
		}

		const FLinearColor ThemeBackground = USlateThemeManager::Get().GetColor(EStyleColor::Background);
		return ThemeBackground.GetLuminance() > Settings.LightThemeLuminanceThreshold;
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
	FTSTicker::FDelegateHandle DynamicContrastTickerHandle;
	FTSTicker::FDelegateHandle WindowsThemeTickerHandle;
	bool bThemeDelegateRegistered = false;
	bool bHasSavedGraphColors = false;
	bool bWindowsThemeReadFailureLogged = false;
	bool bMissingThemeWarningLogged = false;
};

IMPLEMENT_MODULE(FLightThemeFixEditorModule, LightThemeFixEditor)
