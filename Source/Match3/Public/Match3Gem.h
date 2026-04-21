#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Match3Types.h"
#include "Match3Gem.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UMaterialInstanceDynamic;

/** Визуальная 2D-фишка: плоский меш + цвет по EGemType. */
UCLASS()
class MATCH3_API AMatch3Gem : public AActor
{
	GENERATED_BODY()

public:
	AMatch3Gem();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* ClickCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gem")
	EGemType GemType = EGemType::Empty;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gem")
	FIntPoint GridPosition = FIntPoint(-1, -1);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsSelected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsMatched = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsMoving = false;

	UFUNCTION(BlueprintCallable, Category = "Gem")
	void SetGemType(EGemType NewType);

	UFUNCTION(BlueprintCallable, Category = "Gem")
	void SetGridPosition(FIntPoint NewPosition);

	UFUNCTION(BlueprintCallable, Category = "Gem")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Gem")
	void SetMatched(bool bMatched);

	UFUNCTION(BlueprintPure, Category = "Gem")
	EGemType GetGemType() const { return GemType; }

	UFUNCTION(BlueprintPure, Category = "Gem")
	FIntPoint GetGridPosition() const { return GridPosition; }

	UFUNCTION(BlueprintPure, Category = "Gem")
	bool IsMoving() const { return bIsMoving; }

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlaySelectAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayMatchAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayMoveAnimation(FVector TargetPosition, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Gem")
	void Highlight(bool bEnable);

private:
	void EnsureDynamicMaterial();
	static FLinearColor ColorForGemType(EGemType InType);

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GemMID = nullptr;

	FTimerHandle TimerHandle_SelectAnimation;
	FTimerHandle TimerHandle_MoveAnimation;
	float MoveAnimElapsed = 0.f;
};
