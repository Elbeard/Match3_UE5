#include "Match3Grid.h"

#include "Match3Gem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AMatch3Grid::AMatch3Grid()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	GridBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GridBackground"));
	GridBackground->SetupAttachment(SceneRoot);
	GridBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GridLines = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridLines"));
	GridLines->SetupAttachment(SceneRoot);
	GridLines->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		GridBackground->SetStaticMesh(PlaneMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		GridLines->SetStaticMesh(CubeMesh.Object);
	}
}

void AMatch3Grid::BeginPlay()
{
	Super::BeginPlay();
	// ���������� ���� ������ ������, ����� ����� ������ ����.
	RebuildBoardVisual();
	RebuildGridData();
}

FGridCell* AMatch3Grid::CellAt(int32 X, int32 Y)
{
	if (!IsInside(FIntPoint(X, Y)))
	{
		return nullptr;
	}
	const int32 Index = CellIndex(X, Y);
	if (!GridArray.IsValidIndex(Index))
	{
		return nullptr;
	}
	return &GridArray[Index];
}

const FGridCell* AMatch3Grid::CellAt(int32 X, int32 Y) const
{
	if (!IsInside(FIntPoint(X, Y)))
	{
		return nullptr;
	}
	const int32 Index = CellIndex(X, Y);
	if (!GridArray.IsValidIndex(Index))
	{
		return nullptr;
	}
	return &GridArray[Index];
}

FVector AMatch3Grid::WorldLocationForCell(int32 X, int32 Y) const
{
	const float HalfW = GridWidth * CellSize * 0.5f;
	const float HalfH = GridHeight * CellSize * 0.5f;
	const FVector Local(
		X * CellSize - HalfW + CellSize * 0.5f,
		Y * CellSize - HalfH + CellSize * 0.5f,
		6.f); // ���� ���� ����/�����, ����� �������� z-fighting.
	return GetActorTransform().TransformPosition(Local);
}

void AMatch3Grid::RebuildBoardVisual()
{
	const float BoardW = GridWidth * CellSize;
	const float BoardH = GridHeight * CellSize;
	const float HalfW = BoardW * 0.5f;
	const float HalfH = BoardH * 0.5f;

	if (GridBackground)
	{
		// Plane � UE ����� ������� ������ 100x100 uu.
		GridBackground->SetRelativeLocation(FVector(0.f, 0.f, -4.f));
		GridBackground->SetRelativeRotation(FRotator::ZeroRotator);
		GridBackground->SetRelativeScale3D(FVector(BoardW / 100.f, BoardH / 100.f, 1.f));
	}

	if (GridLines && GridLines->GetStaticMesh())
	{
		GridLines->ClearInstances();

		const float Thickness = FMath::Clamp(CellSize * 0.03f, 2.f, 8.f);
		const float Z = -2.f;

		// ������������ ����� �����.
		for (int32 X = 0; X <= GridWidth; ++X)
		{
			const float XPos = -HalfW + X * CellSize;
			const FVector Pos(XPos, 0.f, Z);
			const FVector Scale(Thickness / 100.f, BoardH / 100.f, Thickness / 100.f);
			GridLines->AddInstance(FTransform(FRotator::ZeroRotator, Pos, Scale));
		}

		// �������������� ����� �����.
		for (int32 Y = 0; Y <= GridHeight; ++Y)
		{
			const float YPos = -HalfH + Y * CellSize;
			const FVector Pos(0.f, YPos, Z);
			const FVector Scale(BoardW / 100.f, Thickness / 100.f, Thickness / 100.f);
			GridLines->AddInstance(FTransform(FRotator::ZeroRotator, Pos, Scale));
		}
	}
}

void AMatch3Grid::ClearAllGemActors()
{
	for (FGridCell& Cell : GridArray)
	{
		if (Cell.GemActor)
		{
			Cell.GemActor->Destroy();
			Cell.GemActor = nullptr;
		}
		Cell.GemType = EGemType::Empty;
	}
}

void AMatch3Grid::RebuildGridData()
{
	ClearAllGemActors();
	GridArray.Reset();
	GridArray.Reserve(GridWidth * GridHeight);

	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			// ���������� ��������� ���� ��� ���������� ������.
			const EGemType T = ChooseSpawnType(X, Y);
			FGridCell NewCell(T, FIntPoint(X, Y));
			GridArray.Add(NewCell);
		}
	}

	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			FGridCell* Cell = CellAt(X, Y);
			if (!Cell)
			{
				continue;
			}
			Cell->GemActor = SpawnGemActor(X, Y, Cell->GemType);
		}
	}
}

AMatch3Gem* AMatch3Grid::SpawnGemActor(int32 X, int32 Y, EGemType Type)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMatch3Gem* Gem = World->SpawnActor<AMatch3Gem>(
		AMatch3Gem::StaticClass(),
		WorldLocationForCell(X, Y),
		GetActorRotation(),
		Params);

	if (Gem)
	{
		Gem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		// Plane � UE ~100 uu, ������ ����� �������� 58% ������ ������.
		const float GemDiameter = CellSize * 0.58f;
		const float GemScale = FMath::Max(0.1f, GemDiameter / 100.f);
		Gem->SetActorScale3D(FVector(GemScale));
		Gem->SetGemType(Type);
		Gem->SetGridPosition(FIntPoint(X, Y));
	}

	return Gem;
}

EGemType AMatch3Grid::GetGemTypeAt(int32 X, int32 Y) const
{
	const FGridCell* C = CellAt(X, Y);
	return C ? C->GemType : EGemType::Empty;
}

bool AMatch3Grid::WouldCreateMatchIfPlaced(int32 X, int32 Y, EGemType T) const
{
	if (T == EGemType::Empty)
	{
		return false;
	}

	// ��������� 3 ��������� ���������� �� ����������� ������ (X,Y).
	if (X >= 2 && GetGemTypeAt(X - 1, Y) == T && GetGemTypeAt(X - 2, Y) == T)
	{
		return true;
	}
	if (X >= 1 && X < GridWidth - 1 && GetGemTypeAt(X - 1, Y) == T && GetGemTypeAt(X + 1, Y) == T)
	{
		return true;
	}
	if (X <= GridWidth - 3 && GetGemTypeAt(X + 1, Y) == T && GetGemTypeAt(X + 2, Y) == T)
	{
		return true;
	}

	// ��������� 3 ��������� ���������� �� ��������� ������ (X,Y).
	if (Y >= 2 && GetGemTypeAt(X, Y - 1) == T && GetGemTypeAt(X, Y - 2) == T)
	{
		return true;
	}
	if (Y >= 1 && Y < GridHeight - 1 && GetGemTypeAt(X, Y - 1) == T && GetGemTypeAt(X, Y + 1) == T)
	{
		return true;
	}
	if (Y <= GridHeight - 3 && GetGemTypeAt(X, Y + 1) == T && GetGemTypeAt(X, Y + 2) == T)
	{
		return true;
	}

	return false;
}

EGemType AMatch3Grid::ChooseSpawnType(int32 X, int32 Y) const
{
	TArray<EGemType> Candidates;
	for (int32 E = static_cast<int32>(EGemType::Red); E <= static_cast<int32>(EGemType::Yellow); ++E)
	{
		const EGemType T = static_cast<EGemType>(E);
		if (!WouldCreateMatchIfPlaced(X, Y, T))
		{
			Candidates.Add(T);
		}
	}

	if (Candidates.Num() == 0)
	{
		return static_cast<EGemType>(FMath::RandRange(
			static_cast<int32>(EGemType::Red),
			static_cast<int32>(EGemType::Yellow)));
	}

	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

void AMatch3Grid::SwapCellContents(FIntPoint A, FIntPoint B)
{
	FGridCell* CA = CellAt(A.X, A.Y);
	FGridCell* CB = CellAt(B.X, B.Y);
	if (!CA || !CB)
	{
		return;
	}

	Swap(CA->GemType, CB->GemType);
	Swap(CA->GemActor, CB->GemActor);

	if (CA->GemActor)
	{
		CA->GemActor->SetGridPosition(A);
		CA->GemActor->SetGemType(CA->GemType);
	}
	if (CB->GemActor)
	{
		CB->GemActor->SetGridPosition(B);
		CB->GemActor->SetGemType(CB->GemType);
	}
}

void AMatch3Grid::UpdateGemWorldTransforms(FIntPoint A, FIntPoint B)
{
	if (FGridCell* CA = CellAt(A.X, A.Y))
	{
		if (CA->GemActor)
		{
			CA->GemActor->SetActorLocation(WorldLocationForCell(A.X, A.Y));
		}
	}
	if (FGridCell* CB = CellAt(B.X, B.Y))
	{
		if (CB->GemActor)
		{
			CB->GemActor->SetActorLocation(WorldLocationForCell(B.X, B.Y));
		}
	}
}

void AMatch3Grid::FindAllMatches(TSet<FIntPoint>& OutCells) const
{
	OutCells.Reset();

	// ��������� ����.
	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		int32 X = 0;
		while (X < GridWidth)
		{
			const EGemType T = GetGemTypeAt(X, Y);
			if (T == EGemType::Empty)
			{
				++X;
				continue;
			}

			int32 Run = 1;
			while (X + Run < GridWidth && GetGemTypeAt(X + Run, Y) == T)
			{
				++Run;
			}

			if (Run >= 3)
			{
				for (int32 I = 0; I < Run; ++I)
				{
					OutCells.Add(FIntPoint(X + I, Y));
				}
			}
			X += Run;
		}
	}

	// ��������� �������.
	for (int32 X = 0; X < GridWidth; X++)
	{
		int32 Y = 0;
		while (Y < GridHeight)
		{
			const EGemType T = GetGemTypeAt(X, Y);
			if (T == EGemType::Empty)
			{
				++Y;
				continue;
			}

			int32 Run = 1;
			while (Y + Run < GridHeight && GetGemTypeAt(X, Y + Run) == T)
			{
				++Run;
			}

			if (Run >= 3)
			{
				for (int32 I = 0; I < Run; ++I)
				{
					OutCells.Add(FIntPoint(X, Y + I));
				}
			}
			Y += Run;
		}
	}
}

void AMatch3Grid::ClearCellAt(FIntPoint P)
{
	FGridCell* C = CellAt(P.X, P.Y);
	if (!C)
	{
		return;
	}
	if (C->GemActor)
	{
		C->GemActor->Destroy();
		C->GemActor = nullptr;
	}
	C->GemType = EGemType::Empty;
}

void AMatch3Grid::ApplyGravity()
{
	// ��� ������� ������� "��������" ����� ���� �� write-���������.
	for (int32 X = 0; X < GridWidth; X++)
	{
		int32 WriteY = GridHeight - 1;

		for (int32 ReadY = GridHeight - 1; ReadY >= 0; ReadY--)
		{
			FGridCell* From = CellAt(X, ReadY);
			if (!From || From->GemType == EGemType::Empty)
			{
				continue;
			}

			if (WriteY != ReadY)
			{
				FGridCell* To = CellAt(X, WriteY);
				To->GemType = From->GemType;
				To->GemActor = From->GemActor;
				From->GemType = EGemType::Empty;
				From->GemActor = nullptr;

				if (To->GemActor)
				{
					To->GemActor->SetGridPosition(FIntPoint(X, WriteY));
					To->GemActor->SetActorLocation(WorldLocationForCell(X, WriteY));
				}
			}

			WriteY--;
		}

		for (int32 Y = WriteY; Y >= 0; Y--)
		{
			FGridCell* C = CellAt(X, Y);
			if (C->GemActor)
			{
				C->GemActor->Destroy();
				C->GemActor = nullptr;
			}
			C->GemType = EGemType::Empty;
		}
	}
}

void AMatch3Grid::RefillEmptyCells()
{
	for (int32 Y = 0; Y < GridHeight; Y++)
	{
		for (int32 X = 0; X < GridWidth; X++)
		{
			FGridCell* C = CellAt(X, Y);
			if (!C || C->GemType != EGemType::Empty)
			{
				continue;
			}

			const EGemType NewType = ChooseSpawnType(X, Y);
			C->GemType = NewType;
			C->GemActor = SpawnGemActor(X, Y, NewType);
		}
	}
}

void AMatch3Grid::ResolveCascades()
{
	// ����: ���� -> �������� -> ������� -> ������������, ���� ����� �� ��������.
	while (true)
	{
		TSet<FIntPoint> Matches;
		FindAllMatches(Matches);
		if (Matches.Num() == 0)
		{
			break;
		}

		for (const FIntPoint& P : Matches)
		{
			ClearCellAt(P);
		}

		ApplyGravity();
		RefillEmptyCells();
	}
}

void AMatch3Grid::TrySwapAt(FIntPoint CellA, FIntPoint CellB)
{
	if (bIsResolving)
	{
		return;
	}
	if (!IsInside(CellA) || !IsInside(CellB))
	{
		return;
	}
	if (Manhattan(CellA, CellB) != 1)
	{
		return;
	}

	FGridCell* CA = CellAt(CellA.X, CellA.Y);
	FGridCell* CB = CellAt(CellB.X, CellB.Y);
	if (!CA || !CB)
	{
		return;
	}
	if (CA->GemType == EGemType::Empty || CB->GemType == EGemType::Empty)
	{
		return;
	}

	// ������� �����.
	SwapCellContents(CellA, CellB);
	UpdateGemWorldTransforms(CellA, CellB);

	TSet<FIntPoint> Matches;
	FindAllMatches(Matches);
	// ���� ���� �� ���������, ���������� �����.
	if (Matches.Num() == 0)
	{
		SwapCellContents(CellA, CellB);
		UpdateGemWorldTransforms(CellA, CellB);
		return;
	}

	// ���� ���� ����, ��������� ������ resolve-����.
	bIsResolving = true;
	ResolveCascades();
	bIsResolving = false;
}

void AMatch3Grid::TrySwapGems(AMatch3Gem* GemA, AMatch3Gem* GemB)
{
	if (bIsResolving || !GemA || !GemB || GemA == GemB)
	{
		return;
	}

	const FIntPoint PA = GemA->GetGridPosition();
	const FIntPoint PB = GemB->GetGridPosition();

	const FGridCell* CA = CellAt(PA.X, PA.Y);
	const FGridCell* CB = CellAt(PB.X, PB.Y);
	if (!CA || !CB)
	{
		return;
	}
	if (CA->GemActor != GemA || CB->GemActor != GemB)
	{
		return;
	}

	TrySwapAt(PA, PB);
}
