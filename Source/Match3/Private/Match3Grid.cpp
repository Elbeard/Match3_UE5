#include "Match3Grid.h"

/*
 * Логика поля и ввод (актуальная версия):
 * - В Tick опрашивается игрок 0: ЛКМ + line trace до фишек (стабильно при GameAndUI и видимом курсоре у Spectator).
 * - Тюнинг: FMatch3GameplayTune и при необходимости UMatch3TunePreset (AMatch3Grid::ActiveTune).
 * - Выделение, удержание с покачиванием, обмен кликом по соседу, свайп при отпускании ЛКМ (порог в пикселях из тюнинга).
 */

#include "Match3Gem.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMatch3Grid::AMatch3Grid()
{
	PrimaryActorTick.bCanEverTick = true;

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

void AMatch3Grid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AMatch3Grid::BeginPlay()
{
	Super::BeginPlay();
	RebuildBoardVisual();
	RebuildGridData();
}

void AMatch3Grid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Ввод обрабатывается здесь, а не в PlayerController — сетка всегда «видит» мышь над полем.

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	const bool bLeft = PC->IsInputKeyDown(EKeys::LeftMouseButton);

	const bool bPressed = bLeft && !bWasLMBDownLastTick;
	const bool bReleased = !bLeft && bWasLMBDownLastTick;
	bWasLMBDownLastTick = bLeft;

	if (!CanAcceptInput())
	{
		ClearMouseGesture();
		ClearGemSelectionState();
		return;
	}

	if (bPressed)
	{
		bLeftHeld = true;
		float MX = 0.f;
		float MY = 0.f;
		PC->GetMousePosition(MX, MY);
		const FVector2D Now(MX, MY);

		AMatch3Gem* Gem = nullptr;
		if (TryPickGemWithPC(PC, Gem) && Gem && !Gem->IsMoving())
		{
			if (SelectedGem.IsValid())
			{
				AMatch3Gem* Sel = SelectedGem.Get();
				if (Gem == Sel)
				{
					Gem->SetSelectionPressActive(true);
					bPressActive = true;
					PressScreenPos = Now;
					DraggedGem = Gem;
				}
				else if (Manhattan(Sel->GetGridPosition(), Gem->GetGridPosition()) == 1)
				{
					Sel->SetSelectionPressActive(false);
					Sel->SetSelected(false);
					SelectedGem = nullptr;
					DraggedGem = nullptr;
					bPressActive = false;
					TrySwapGems(Sel, Gem);
				}
				else
				{
					Sel->SetSelectionPressActive(false);
					Sel->SetSelected(false);
					Gem->SetSelected(true);
					SelectedGem = Gem;
					Gem->SetSelectionPressActive(true);
					bPressActive = true;
					PressScreenPos = Now;
					DraggedGem = Gem;
				}
			}
			else
			{
				Gem->SetSelected(true);
				SelectedGem = Gem;
				Gem->SetSelectionPressActive(true);
				bPressActive = true;
				PressScreenPos = Now;
				DraggedGem = Gem;
			}
		}
		else
		{
			if (SelectedGem.IsValid())
			{
				if (AMatch3Gem* S = SelectedGem.Get())
				{
					S->SetSelectionPressActive(false);
					S->SetSelected(false);
				}
				SelectedGem = nullptr;
			}
			if (DraggedGem.IsValid())
			{
				if (AMatch3Gem* D = DraggedGem.Get())
				{
					D->SetSelectionPressActive(false);
				}
			}
			DraggedGem = nullptr;
			bPressActive = false;
		}
	}

	if (bReleased)
	{
		bLeftHeld = false;

		if (bPressActive)
		{
			AMatch3Gem* FromGem = DraggedGem.Get();
			if (FromGem)
			{
				FromGem->SetSelectionPressActive(false);
			}
			bPressActive = false;
			DraggedGem = nullptr;

			if (FromGem && !FromGem->IsMoving() && SelectedGem.Get() == FromGem)
			{
				float MX = 0.f;
				float MY = 0.f;
				PC->GetMousePosition(MX, MY);
				const FVector2D Now(MX, MY);
				const FVector2D Drag = Now - PressScreenPos;

				if (Drag.Size() >= ActiveTune().InputMinSwipePixels)
				{
					FIntPoint From = FromGem->GetGridPosition();
					FIntPoint Delta(0, 0);
					if (FMath::Abs(Drag.X) >= FMath::Abs(Drag.Y))
					{
						Delta.X = Drag.X > 0.f ? 1 : -1;
					}
					else
					{
						Delta.Y = Drag.Y > 0.f ? 1 : -1;
					}
					TrySwapAt(From, From + Delta);
				}
			}
		}
	}
}

void AMatch3Grid::ClearGemSelectionState()
{
	if (SelectedGem.IsValid())
	{
		if (AMatch3Gem* G = SelectedGem.Get())
		{
			G->SetSelectionPressActive(false);
			G->SetSelected(false);
		}
	}
	SelectedGem = nullptr;
	if (DraggedGem.IsValid())
	{
		if (AMatch3Gem* D = DraggedGem.Get())
		{
			D->SetSelectionPressActive(false);
		}
	}
	DraggedGem = nullptr;
	bPressActive = false;
}

FMatch3GameplayTune AMatch3Grid::GetResolvedTune() const
{
	return ActiveTune();
}

void AMatch3Grid::ClearMouseGesture()
{
	if (AMatch3Gem* G = DraggedGem.Get())
	{
		G->SetSelectionPressActive(false);
	}
	DraggedGem = nullptr;
	bPressActive = false;
	bLeftHeld = false;
}

bool AMatch3Grid::TryPickGemWithPC(APlayerController* PC, AMatch3Gem*& OutGem)
{
	OutGem = nullptr;
	if (!PC)
	{
		return false;
	}

	UWorld* W = GetWorld();
	if (!W)
	{
		return false;
	}

	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit))
	{
		if (AMatch3Gem* Gem = Cast<AMatch3Gem>(Hit.GetActor()))
		{
			OutGem = Gem;
			return true;
		}
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * ActiveTune().PickLineTraceLengthUU;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Match3GridPickGem), true);
	if (APawn* Pawn = PC->GetPawn())
	{
		Params.AddIgnoredActor(Pawn);
	}

	TArray<FHitResult> Hits;
	if (W->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ECC_Visibility, Params))
	{
		for (const FHitResult& H : Hits)
		{
			if (AMatch3Gem* Gem = Cast<AMatch3Gem>(H.GetActor()))
			{
				OutGem = Gem;
				return true;
			}
		}
	}

	return false;
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
	const float HalfW = GridW() * CellSz() * 0.5f;
	const float HalfH = GridH() * CellSz() * 0.5f;
	const FVector Local(
		X * CellSz() - HalfW + CellSz() * 0.5f,
		Y * CellSz() - HalfH + CellSz() * 0.5f,
		ActiveTune().GemCellZOffset);
	return GetActorTransform().TransformPosition(Local);
}

void AMatch3Grid::RebuildBoardVisual()
{
	const FMatch3GameplayTune& T = ActiveTune();
	const float BoardW = GridW() * CellSz();
	const float BoardH = GridH() * CellSz();
	const float HalfW = BoardW * 0.5f;
	const float HalfH = BoardH * 0.5f;

	if (GridBackground)
	{
		// Меш Plane из BasicShapes по умолчанию 100×100 uu.
		GridBackground->SetRelativeLocation(FVector(0.f, 0.f, T.BoardBackgroundZOffset));
		GridBackground->SetRelativeRotation(FRotator::ZeroRotator);
		GridBackground->SetRelativeScale3D(FVector(BoardW / 100.f, BoardH / 100.f, 1.f));
	}

	if (GridLines && GridLines->GetStaticMesh())
	{
		GridLines->ClearInstances();

		const float Thickness = FMath::Clamp(
			CellSz() * T.GridLineThicknessFactor,
			T.GridLineThicknessMin,
			T.GridLineThicknessMax);
		const float Z = T.GridLinesZ;

		for (int32 X = 0; X <= GridW(); ++X)
		{
			const float XPos = -HalfW + X * CellSz();
			const FVector Pos(XPos, 0.f, Z);
			const FVector Scale(Thickness / 100.f, BoardH / 100.f, Thickness / 100.f);
			GridLines->AddInstance(FTransform(FRotator::ZeroRotator, Pos, Scale));
		}

		for (int32 Y = 0; Y <= GridH(); ++Y)
		{
			const float YPos = -HalfH + Y * CellSz();
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
	GridArray.Reserve(GridW() * GridH());

	for (int32 Y = 0; Y < GridH(); Y++)
	{
		for (int32 X = 0; X < GridW(); X++)
		{
			// Тип, при котором не образуется «три в ряд» сразу после появления.
			const EGemType T = ChooseSpawnType(X, Y);
			FGridCell NewCell(T, FIntPoint(X, Y));
			GridArray.Add(NewCell);
		}
	}

	for (int32 Y = 0; Y < GridH(); Y++)
	{
		for (int32 X = 0; X < GridW(); X++)
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
		// Тот же Plane ~100 uu; масштаб актора из GemDiameterFractionOfCell относительно клетки.
		const float GemDiameter = CellSz() * ActiveTune().GemDiameterFractionOfCell;
		const float GemScale = FMath::Max(0.1f, GemDiameter / 100.f);
		Gem->SetActorScale3D(FVector(GemScale));
		Gem->SetGemType(Type);
		Gem->SetGridPosition(FIntPoint(X, Y));
		Gem->ApplyTune(GetResolvedTune());
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

	// Даст ли размещение типа T в (X,Y) три и более подряд по горизонтали?
	if (X >= 2 && GetGemTypeAt(X - 1, Y) == T && GetGemTypeAt(X - 2, Y) == T)
	{
		return true;
	}
	if (X >= 1 && X < GridW() - 1 && GetGemTypeAt(X - 1, Y) == T && GetGemTypeAt(X + 1, Y) == T)
	{
		return true;
	}
	if (X <= GridW() - 3 && GetGemTypeAt(X + 1, Y) == T && GetGemTypeAt(X + 2, Y) == T)
	{
		return true;
	}

	// Даст ли размещение типа T в (X,Y) три и более подряд по вертикали?
	if (Y >= 2 && GetGemTypeAt(X, Y - 1) == T && GetGemTypeAt(X, Y - 2) == T)
	{
		return true;
	}
	if (Y >= 1 && Y < GridH() - 1 && GetGemTypeAt(X, Y - 1) == T && GetGemTypeAt(X, Y + 1) == T)
	{
		return true;
	}
	if (Y <= GridH() - 3 && GetGemTypeAt(X, Y + 1) == T && GetGemTypeAt(X, Y + 2) == T)
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

	// Горизонтальные серии из 3+ одинаковых фишек.
	for (int32 Y = 0; Y < GridH(); Y++)
	{
		int32 X = 0;
		while (X < GridW())
		{
			const EGemType T = GetGemTypeAt(X, Y);
			if (T == EGemType::Empty)
			{
				++X;
				continue;
			}

			int32 Run = 1;
			while (X + Run < GridW() && GetGemTypeAt(X + Run, Y) == T)
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

	// Вертикальные серии из 3+ одинаковых фишек.
	for (int32 X = 0; X < GridW(); X++)
	{
		int32 Y = 0;
		while (Y < GridH())
		{
			const EGemType T = GetGemTypeAt(X, Y);
			if (T == EGemType::Empty)
			{
				++Y;
				continue;
			}

			int32 Run = 1;
			while (Y + Run < GridH() && GetGemTypeAt(X, Y + Run) == T)
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
	// Уплотнение столбца снизу вверх: указатели чтения ReadY и записи WriteY без отдельного буфера.
	for (int32 X = 0; X < GridW(); X++)
	{
		int32 WriteY = GridH() - 1;

		for (int32 ReadY = GridH() - 1; ReadY >= 0; ReadY--)
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
	for (int32 Y = 0; Y < GridH(); Y++)
	{
		for (int32 X = 0; X < GridW(); X++)
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
	// Цикл: матчи → очистка → гравитация → досыпание, пока на поле есть матчи.
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

	// Пробный обмен: проверяем, появятся ли матчи.
	SwapCellContents(CellA, CellB);
	UpdateGemWorldTransforms(CellA, CellB);

	TSet<FIntPoint> Matches;
	FindAllMatches(Matches);
	// Матчей нет — откатываем обмен.
	if (Matches.Num() == 0)
	{
		SwapCellContents(CellA, CellB);
		UpdateGemWorldTransforms(CellA, CellB);
		return;
	}

	ClearGemSelectionState();

	// Матчи есть — полное разрешение каскадов.
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
