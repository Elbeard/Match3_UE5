#include "Match3GameMode.h"
#include "Match3Grid.h"
#include "Match3PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"

AMatch3GameMode::AMatch3GameMode()
{
	// Контроллер: только камера над полем; ввод по фишкам — в AMatch3Grid::Tick.
	PlayerControllerClass = AMatch3PlayerController::StaticClass();
	// Камера Spectator (при желании WASD); поворот мышью отключён в PlayerController.
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	// По умолчанию автоспавним C++ сетку.
	GridClass = AMatch3Grid::StaticClass();
}

void AMatch3GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld())
	{
		return;
	}

	// Если сетку уже положили на карту вручную, ничего не спавним.
	AActor* ExistingGrid = UGameplayStatics::GetActorOfClass(GetWorld(), AMatch3Grid::StaticClass());
	if (ExistingGrid)
	{
		return;
	}

	if (!GridClass)
	{
		return;
	}

	// Спавним поле, если на карте нет вручную размещенного AMatch3Grid.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AMatch3Grid>(
		GridClass,
		AutoSpawnGridLocation,
		FRotator::ZeroRotator,
		SpawnParams);
}
