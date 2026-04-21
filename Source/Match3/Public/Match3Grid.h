#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Match3Types.h"
#include "Match3Grid.generated.h"

class AMatch3Gem;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY()

	// ���������� ��� ����� � ������.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match3")
	EGemType GemType = EGemType::Empty;

	// ������ �� ���������� ����� ����� � ����.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match3")
	TObjectPtr<AMatch3Gem> GemActor = nullptr;

	// ���������� ������ � �����.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match3")
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	FGridCell() = default;
	FGridCell(EGemType InType, FIntPoint InPos)
		: GemType(InType), GridPosition(InPos) {}
};

/** �������� ����� ����: ������ ������ match-3 � ��������� �������� �����. */
UCLASS(Blueprintable)
class MATCH3_API AMatch3Grid : public AActor
{
	GENERATED_BODY()

public:
	AMatch3Grid();

	virtual void BeginPlay() override;

	/** ���� ���� ������������ ������, ����� ���� �����������. */
	UFUNCTION(BlueprintPure, Category = "Match3")
	bool CanAcceptInput() const { return !bIsResolving; }

	/** ����� ���� ����� �� �� ������� (� ��������� ���������). */
	UFUNCTION(BlueprintCallable, Category = "Match3")
	void TrySwapGems(AMatch3Gem* GemA, AMatch3Gem* GemB);

	/** ����� ���� ������ �� �����������, ���� Manhattan distance == 1. */
	UFUNCTION(BlueprintCallable, Category = "Match3")
	void TrySwapAt(FIntPoint CellA, FIntPoint CellB);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Setup", meta = (ClampMin = "3", ClampMax = "32"))
	int32 GridWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Setup", meta = (ClampMin = "3", ClampMax = "32"))
	int32 GridHeight = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Setup", meta = (ClampMin = "10"))
	float CellSize = 120.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	// ������-��� ��� ������.
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<UStaticMeshComponent> GridBackground = nullptr;

	// �������� ����� ����� (������ � ������ �� draw calls).
	UPROPERTY(VisibleAnywhere, Category = "Grid|Visual")
	TObjectPtr<UInstancedStaticMeshComponent> GridLines = nullptr;

	UPROPERTY()
	TArray<FGridCell> GridArray;

	// ���� ��������� "���� resolve ��������".
	bool bIsResolving = false;

	FORCEINLINE bool IsInside(FIntPoint P) const
	{
		return P.X >= 0 && P.X < GridWidth && P.Y >= 0 && P.Y < GridHeight;
	}

	FORCEINLINE static int32 Manhattan(FIntPoint A, FIntPoint B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

	FORCEINLINE int32 CellIndex(int32 X, int32 Y) const { return Y * GridWidth + X; }

	FGridCell* CellAt(int32 X, int32 Y);
	const FGridCell* CellAt(int32 X, int32 Y) const;

public:
	FGridCell* GetCell(int32 X, int32 Y) { return CellAt(X, Y); }
	const FGridCell* GetCell(int32 X, int32 Y) const { return CellAt(X, Y); }

private:
	// ������������� ����������� ���� � ���������� ������� �����.
	void RebuildGridData();
	// ������� ������� ������ ����� ��� ����������� ����.
	void ClearAllGemActors();
	// ������ 2D ��� � ������� ����� ��� �������.
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
};
