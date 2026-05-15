// Общие типы Match-3 (заголовки Gem и Grid независимы друг от друга).

#pragma once

#include "CoreMinimal.h"
#include "Match3Types.generated.h"

/** Empty = нет фишки; остальные — четыре цвета (без фиолетового и т.д.). */
UENUM(BlueprintType)
enum class EGemType : uint8
{
	Empty  UMETA(DisplayName = "Empty"),
	Red  UMETA(DisplayName = "Red"),
	Blue  UMETA(DisplayName = "Blue"),
	Green  UMETA(DisplayName = "Green"),
	Yellow UMETA(DisplayName = "Yellow"),
};
