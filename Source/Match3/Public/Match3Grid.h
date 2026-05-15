#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Match3Config.h"
#include "Match3Types.h"
#include "Match3Grid.generated.h"

class AMatch3Gem;
class APlayerController;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY()

	// Тип гема в этой ячейке.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match3")
	EGemType GemType = EGemType::Empty;

	// Заспавненный актор фишки (nullptr, если ячейка пустая).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match3")
	TObjectPtr<AMatch3Gem> GemActor = nullptr;

	// Координата ячейки в сетке.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match3")
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	FGridCell() = default;
	FGridCell(EGemType InType, FIntPoint InPos)
		: GemType(InType), GridPosition(InPos) {}
};

/** Актор игрового поля: правила match-3 и размещение фишек. */
UCLASS(Blueprintable)
class MATCH3_API AMatch3Grid : public AActor
{
	GENERATED_BODY()

public:
	AMatch3Grid();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

	/** Пока идёт разрешение каскадов, ввод игрока игнорируется. */
	UFUNCTION(BlueprintPure, Category = "Match3")
	bool CanAcceptInput() const { return !bIsResolving; }

	/** Обмен двух фишек по указателям на акторы (должны быть соседями на сетке). */
	UFUNCTION(BlueprintCallable, Category = "Match3")
	void TrySwapGems(AMatch3Gem* GemA, AMatch3Gem* GemB);

	/** Обмен двух клеток по индексам, если расстояние Манхэттена == 1. */
	UFUNCTION(BlueprintCallable, Category = "Match3")
	void TrySwapAt(FIntPoint CellA, FIntPoint CellB);

	/** Копия активного тюнинга (удобно для Blueprint). */
	UFUNCTION(BlueprintPure, Category = "Match3")
	FMatch3GameplayTune GetResolvedTune() const;

protected:
	/**
	 * Опциональный пресет (аналог Unity ScriptableObject).
	 * Content Browser → ПКМ → Miscellaneous → Data Asset → Match3 Tune Preset.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Match3|Tune",
		meta = (ToolTip = "Если назначен — используются поля из ассета, Local Tune скрыт."))
	TObjectPtr<UMatch3TunePreset> TunePreset;

	/** Настройка в Details на акторе сетки (пока Tune Preset пустой). */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Match3|Tune",
		meta = (
			EditCondition = "TunePreset == nullptr",
			EditConditionHides,
			ToolTip = "Значения на экземпляре сетки; не используются, если задан Tune Preset."))
	FMatch3GameplayTune LocalTune;

	FORCEINLINE const FMatch3GameplayTune& ActiveTune() const
	{
		if (TunePreset)
		{
			return TunePreset->Tune;
		}
		return LocalTune;
	}

	FORCEINLINE int32 GridW() const { return ActiveTune().GridWidth; }
	FORCEINLINE int32 GridH() const { return ActiveTune().GridHeight; }
	FORCEINLINE float CellSz() const { return ActiveTune().CellSize; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	// Плоскость фона на всю доску.
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<UStaticMeshComponent> GridBackground = nullptr;

	// Линии сетки через инстансинг (меньше проходов отрисовки).
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> GridLines = nullptr;

	UPROPERTY()
	TArray<FGridCell> GridArray;

	// Истина, пока выполняется разрешение матчей / каскадов.
	bool bIsResolving = false;

	/** Ввод обрабатывается на сетке — работает даже если PlayerController не получает события. */
	bool bWasLMBDownLastTick = false;
	bool bLeftHeld = false;
	bool bPressActive = false;
	FVector2D PressScreenPos = FVector2D::ZeroVector;
	TWeakObjectPtr<AMatch3Gem> DraggedGem;
	/** Фишка, выбранная одним кликом для обмена с соседней (анимация покачивания только при удержании ЛКМ). */
	TWeakObjectPtr<AMatch3Gem> SelectedGem;

	FORCEINLINE bool IsInside(FIntPoint P) const
	{
		return P.X >= 0 && P.X < GridW() && P.Y >= 0 && P.Y < GridH();
	}

	FORCEINLINE static int32 Manhattan(FIntPoint A, FIntPoint B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

	FORCEINLINE int32 CellIndex(int32 X, int32 Y) const { return Y * GridW() + X; }

	FGridCell* CellAt(int32 X, int32 Y);
	const FGridCell* CellAt(int32 X, int32 Y) const;

public:
	FGridCell* GetCell(int32 X, int32 Y) { return CellAt(X, Y); }
	const FGridCell* GetCell(int32 X, int32 Y) const { return CellAt(X, Y); }

private:
	// Заполняет GridArray по тюнингу и спавнит фишки.
	void RebuildGridData();
	// Удаляет все акторы фишек перед пересборкой данных сетки.
	void ClearAllGemActors();
	// Масштабирует фон и пересобирает инстансы линий под текущий размер поля.
	void RebuildBoardVisual();

	FVector WorldLocationForCell(int32 X, int32 Y) const;
	void UpdateGemWorldTransforms(FIntPoint A, FIntPoint B);

	AMatch3Gem* SpawnGemActor(int32 X, int32 Y, EGemType Type);

	void SwapCellContents(FIntPoint A, FIntPoint B);

	EGemType GetGemTypeAt(int32 X, int32 Y) const;
	bool WouldCreateMatchIfPlaced(int32 X, int32 Y, EGemType Candidate) const;
	EGemType ChooseSpawnType(int32 X, int32 Y) const;

	void FindAllMatches(TSet<FIntPoint>& OutCells) const;
	void ClearCellAt(FIntPoint P);
	void ApplyGravity();
	void RefillEmptyCells();
	void ResolveCascades();

	void ClearMouseGesture();
	bool TryPickGemWithPC(APlayerController* PC, AMatch3Gem*& OutGem);
	void ClearGemSelectionState();
};
