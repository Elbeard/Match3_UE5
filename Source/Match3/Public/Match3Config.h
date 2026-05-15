#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Match3Config.generated.h"

/**
 * Все настраиваемые параметры Match3 в одной структуре.
 * AMatch3Grid берёт значения из Local Tune и/или UMatch3TunePreset; AMatch3Gem::ApplyTune читает ту же структуру с родительской сетки.
 */
USTRUCT(BlueprintType)
struct FMatch3GameplayTune
{
	GENERATED_BODY()

	// --- Grid|Layout ---
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Grid|Layout",
		meta = (ClampMin = "3", ClampMax = "32"))
	int32 GridWidth = 8;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Grid|Layout",
		meta = (ClampMin = "3", ClampMax = "32"))
	int32 GridHeight = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Layout", meta = (ClampMin = "10"))
	float CellSize = 120.f;

	// --- Grid|Visual ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ToolTip = "Z подложки (плоскость поля), uu."))
	float BoardBackgroundZOffset = -4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ToolTip = "Z линий сетки, uu."))
	float GridLinesZ = -2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ClampMin = "0.001", ToolTip = "Толщина линий ≈ CellSize * этот множитель."))
	float GridLineThicknessFactor = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ClampMin = "0.1"))
	float GridLineThicknessMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ClampMin = "0.1"))
	float GridLineThicknessMax = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Visual", meta = (ToolTip = "Z камней в локали сетки (uu)."))
	float GemCellZOffset = 6.f;

	// --- Grid|Input ---
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Grid|Input",
		meta = (ClampMin = "5", ToolTip = "Минимальный свайп в пикселях."))
	float InputMinSwipePixels = 24.f;

	// --- Grid|Trace ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Trace", meta = (ClampMin = "1000"))
	float PickLineTraceLengthUU = 1000000.f;

	// --- Gem|Mesh ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Mesh", meta = (ClampMin = "0.01", ClampMax = "2", ToolTip = "Масштаб plane меша по X/Y (локально)."))
	float GemPlaneScaleXY = 0.85f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Mesh",
		meta = (ClampMin = "0.05", ClampMax = "1", ToolTip = "Диаметр камня относительно размера клетки."))
	float GemDiameterFractionOfCell = 0.58f;

	// --- Gem|Colors (Linear) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Colors")
	FLinearColor GemColorRed = FLinearColor(0.92f, 0.18f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Colors")
	FLinearColor GemColorBlue = FLinearColor(0.18f, 0.45f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Colors")
	FLinearColor GemColorGreen = FLinearColor(0.22f, 0.82f, 0.32f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Colors")
	FLinearColor GemColorYellow = FLinearColor(0.95f, 0.86f, 0.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Colors")
	FLinearColor GemColorEmpty = FLinearColor(0.65f, 0.65f, 0.65f);

	// --- Gem|Selection ---
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Selection",
		meta = (ClampMin = "0", ToolTip = "Амплитуда «качка» по Z при удержании ЛКМ (uu)."))
	float SelectionBobAmplitudeUU = 1.33f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Selection",
		meta = (ClampMin = "0.1", ClampMax = "8", ToolTip = "Частота пульса при удержании (Гц)."))
	float SelectionPulseHz = 1.15f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Selection",
		meta = (ClampMin = "0", ClampMax = "0.25", ToolTip = "Масштаб ~ 1 ± это значение."))
	float SelectionScaleAmp = 0.04f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Selection",
		meta = (ClampMin = "0", ToolTip = "Покачивание в плоскости XY (uu)."))
	float SelectionWobbleXYUU = 0.83f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Selection",
		meta = (ClampMin = "1", ToolTip = "Частота обновления покачивания (Гц), таймер 1/TickHz."))
	float SelectionUpdateHz = 60.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Gem|Pick",
		meta = (ClampMin = "1", ToolTip = "Радиус сферы клика вокруг фишки (uu)."))
	float GemClickRadiusUU = 48.f;

	// --- Gem|Animation ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Animation", meta = (ClampMin = "1.0", ToolTip = "Множитель масштаба при коротком «пульсе» выбора."))
	float SelectAnimScaleMultiplier = 1.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Animation", meta = (ClampMin = "0.01"))
	float SelectAnimDurationSec = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Animation", meta = (ClampMin = "0.01"))
	float MatchBusyDurationSec = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Animation", meta = (ClampMin = "1", ToolTip = "Шаг таймера анимации перемещения (Гц)."))
	float MoveAnimTickHz = 60.f;

	// --- Gem|Highlight ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 CustomDepthStencilValue = 1;
};

/** Пресет (Unity ScriptableObject): Content → Data Asset → Match3 Tune Preset. */
UCLASS(BlueprintType)
class MATCH3_API UMatch3TunePreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match3")
	FMatch3GameplayTune Tune;
};
