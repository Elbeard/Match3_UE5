#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Match3PlayerController.generated.h"

class AMatch3Grid;

/**
 * Выравнивает камеру Spectator над полем; мышиный ввод и выбор фишек — в AMatch3Grid::Tick.
 * Поворот камеры мышью отключён (SetIgnoreLookInput).
 */
UCLASS()
class MATCH3_API AMatch3PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMatch3PlayerController();

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void CacheGridIfNeeded();
	AMatch3Grid* ResolveGrid() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AMatch3Grid> CachedGrid;

	UPROPERTY(EditAnywhere, Category = "Match3|Camera", meta = (ClampMin = "1.0"))
	float CameraHeightMultiplier = 2.1f;

	UPROPERTY(EditAnywhere, Category = "Match3|Camera", meta = (ClampMin = "100.0"))
	float CameraHeightOffset = 350.f;

	UPROPERTY(EditAnywhere, Category = "Match3|Camera", meta = (ClampMin = "-89.0", ClampMax = "-30.0"))
	float CameraPitch = -82.f;

	bool bCameraAlignedToGrid = false;
};
