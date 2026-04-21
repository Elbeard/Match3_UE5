#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Match3GameMode.generated.h"

class AMatch3Grid;

/**
 * Назначает наш PlayerController со свайпом по умолчанию.
 * Если сетки на уровне нет, GameMode создаст её автоматически.
 */
UCLASS()
class MATCH3_API AMatch3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMatch3GameMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Match3")
	TSubclassOf<AMatch3Grid> GridClass;

	UPROPERTY(EditDefaultsOnly, Category = "Match3")
	FVector AutoSpawnGridLocation = FVector::ZeroVector;
};
