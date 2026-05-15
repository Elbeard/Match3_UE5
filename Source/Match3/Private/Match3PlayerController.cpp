#include "Match3PlayerController.h"

#include "Match3Grid.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMatch3PlayerController::AMatch3PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
	AutoReceiveInput = EAutoReceiveInput::Player0;
}

void AMatch3PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Фиксированный вид на доску: у Spectator по умолчанию отключаем вращение камеры мышью.
	SetIgnoreLookInput(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMatch3PlayerController::CacheGridIfNeeded()
{
	if (CachedGrid.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		AMatch3Grid* Found = Cast<AMatch3Grid>(
			UGameplayStatics::GetActorOfClass(World, AMatch3Grid::StaticClass()));
		if (Found)
		{
			CachedGrid = Found;
		}
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
	if (!Grid)
	{
		return;
	}

	if (!bCameraAlignedToGrid)
	{
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn)
		{
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
}
