#include "Match3PlayerController.h"

#include "Match3Grid.h"
#include "Match3Gem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AMatch3PlayerController::AMatch3PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMatch3PlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
}

void AMatch3PlayerController::CacheGridIfNeeded()
{
	if (CachedGrid.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// Находим первую сетку на уровне и кэшируем, чтобы не искать каждый кадр.
		AMatch3Grid* Found = Cast<AMatch3Grid>(
			UGameplayStatics::GetActorOfClass(World, AMatch3Grid::StaticClass()));
		CachedGrid = Found;
	}
}

AMatch3Grid* AMatch3PlayerController::ResolveGrid() const
{
	if (CachedGrid.IsValid())
	{
		return CachedGrid.Get();
	}
	if (UWorld* World = GetWorld())
	{
		return Cast<AMatch3Grid>(
			UGameplayStatics::GetActorOfClass(World, AMatch3Grid::StaticClass()));
	}
	return nullptr;
}

void AMatch3PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	CacheGridIfNeeded();
	AMatch3Grid* Grid = ResolveGrid();
	if (!Grid || !Grid->CanAcceptInput())
	{
		bPressActive = false;
		PressedGem = nullptr;
		return;
	}

	if (!bCameraAlignedToGrid)
	{
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn)
		{
			// Автоматически ставим камеру над полем один раз при старте.
			FVector BoundsOrigin = FVector::ZeroVector;
			FVector BoundsExtent = FVector::ZeroVector;
			Grid->GetActorBounds(false, BoundsOrigin, BoundsExtent);

			const float MaxHalfSize = FMath::Max(BoundsExtent.X, BoundsExtent.Y);
			const float CameraZ = MaxHalfSize * CameraHeightMultiplier + CameraHeightOffset;
			const FVector CameraLocation = BoundsOrigin + FVector(0.f, 0.f, CameraZ);
			const FRotator CameraRotation(CameraPitch, 0.f, 0.f);

			ControlledPawn->SetActorLocation(CameraLocation);
			ControlledPawn->SetActorRotation(CameraRotation);
			SetControlRotation(CameraRotation);
			bCameraAlignedToGrid = true;
		}
	}

	const bool bLeftDown = IsInputKeyDown(EKeys::LeftMouseButton);
	// Считаем "нажатие" и "отпускание" вручную из состояния прошлой рамки.
	const bool bLeftPressed = bLeftDown && !bWasLeftDownLastTick;
	const bool bLeftReleased = !bLeftDown && bWasLeftDownLastTick;

	float MX = 0.f;
	float MY = 0.f;
	GetMousePosition(MX, MY);
	const FVector2D Now(MX, MY);

	if (bLeftPressed)
	{
		// На старте drag запоминаем фишку под курсором.
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, true, Hit))
		{
			if (AMatch3Gem* Gem = Cast<AMatch3Gem>(Hit.GetActor()))
			{
				if (!Gem->IsMoving())
				{
					bPressActive = true;
					PressScreenPos = Now;
					PressedGem = Gem;
				}
			}
		}
	}

	if (bPressActive && bLeftReleased)
	{
		bPressActive = false;

		AMatch3Gem* FromGem = PressedGem.Get();
		PressedGem = nullptr;
		if (!FromGem)
		{
			return;
		}

		const FVector2D Drag = Now - PressScreenPos;

		// 1) Если попали отпусканием по другой фишке и она соседняя — меняемся с ней.
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, true, Hit))
		{
			if (AMatch3Gem* ToGem = Cast<AMatch3Gem>(Hit.GetActor()))
			{
				if (ToGem != FromGem && !ToGem->IsMoving())
				{
					Grid->TrySwapGems(FromGem, ToGem);
					return;
				}
			}
		}

		// 2) Иначе — свайп по экрану (доминирующая ось).
		if (Drag.Size() < MinSwipePixels)
		{
			return;
		}

		FIntPoint From = FromGem->GetGridPosition();
		FIntPoint Delta(0, 0);
		if (FMath::Abs(Drag.X) >= FMath::Abs(Drag.Y))
		{
			Delta.X = Drag.X > 0.f ? 1 : -1;
		}
		else
		{
			// Экран: Y вниз растёт; для «свайпа вверх» Drag.Y отрицательный — двигаемся к меньшему Y в сетке.
			Delta.Y = Drag.Y > 0.f ? 1 : -1;
		}

		const FIntPoint Target = From + Delta;
		Grid->TrySwapAt(From, Target);
	}

	// Если отпустили кнопку вне логики выше
	if (!bLeftDown && bPressActive)
	{
		bPressActive = false;
		PressedGem = nullptr;
	}

	bWasLeftDownLastTick = bLeftDown;
}
