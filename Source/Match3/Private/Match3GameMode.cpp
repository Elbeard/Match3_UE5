#include "Match3GameMode.h"
#include "Match3Grid.h"
#include "Match3PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"

AMatch3GameMode::AMatch3GameMode()
{
	// Контроллер обрабатывает mouse swipe / click логику.
	PlayerControllerClass = AMatch3PlayerController::StaticClass();
	// Чтобы в PIE была камера: летаете WASD + мышь, подлетаете к полю сверху.
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
