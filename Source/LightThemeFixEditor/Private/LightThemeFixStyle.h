#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ULightThemeFixSettings;

/** Owns the FAppStyle overrides installed by LightThemeFix. */
class FLightThemeFixStyle final
{
public:
	void Initialize(const ULightThemeFixSettings& Settings);
	void Shutdown();

private:
	FName PreviousAppStyleName;
	TSharedPtr<FSlateStyleSet> Style;
};
