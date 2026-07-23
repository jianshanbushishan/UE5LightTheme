#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ULightThemeFixSettings;

/** Owns the FAppStyle overrides installed by LightThemeFix. */
class FLightThemeFixStyle final
{
public:
	void Initialize(const ULightThemeFixSettings& Settings);
	void ApplyTheme(const ULightThemeFixSettings& Settings, bool bEnableLightThemeFixes);
	void Shutdown();

private:
	FName PreviousAppStyleName;
	TSharedPtr<FSlateStyleSet> Style;
	TArray<FName> FocusedSpinBoxStyleNames;
	TArray<FName> FocusedEditableTextBoxStyleNames;
	TArray<FName> FocusedSearchBoxStyleNames;
	TArray<FName> FocusedInlineTextStyleNames;
};
