#include "Match3Gem.h"

#include "Match3Grid.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

namespace
{
	/** BasicShapeMaterial и другие встроенные материалы по версиям UE называют цветовой вектор по-разному. */
	void ApplyGemTint(UMaterialInstanceDynamic* MID, const FLinearColor& Color)
	{
		if (!MID)
		{
			return;
		}

		static const FName VectorNames[] = {
			FName(TEXT("BaseColor")),
			FName(TEXT("Color")),
			FName(TEXT("Tint")),
			FName(TEXT("Diffuse")),
		};

		for (const FName& N : VectorNames)
		{
			MID->SetVectorParameterValue(N, Color);
		}
	}
}

AMatch3Gem::AMatch3Gem()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ClickCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ClickCollision"));
	ClickCollision->SetupAttachment(MeshComponent);
	ClickCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ApplyTune(FMatch3GameplayTune());

	// Игра на плоскости: используем меш Plane из движка, а не сферу.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(PlaneAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMat.Succeeded())
	{
		MeshComponent->SetMaterial(0, ShapeMat.Object);
	}
}

void AMatch3Gem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshTuneFromParentGrid();
}

void AMatch3Gem::BeginPlay()
{
	Super::BeginPlay();
	RefreshTuneFromParentGrid();
	if (MeshComponent)
	{
		MeshBaseRelativeScale = MeshComponent->GetRelativeScale3D();
	}
	SetGemType(GemType);
}

void AMatch3Gem::ApplyTune(const FMatch3GameplayTune& Tune)
{
	GemTune = Tune;

	if (ClickCollision)
	{
		ClickCollision->SetSphereRadius(GemTune.GemClickRadiusUU);
	}

	if (MeshComponent)
	{
		MeshComponent->SetRelativeScale3D(FVector(GemTune.GemPlaneScaleXY, GemTune.GemPlaneScaleXY, 1.f));
	}

	if (GemMID)
	{
		ApplyGemTint(GemMID, ResolveGemColor(GemType));
	}
}

void AMatch3Gem::RefreshTuneFromParentGrid()
{
	if (AMatch3Grid* Grid = Cast<AMatch3Grid>(GetAttachParentActor()))
	{
		ApplyTune(Grid->GetResolvedTune());
	}
}

void AMatch3Gem::UpdateSelectionVisual()
{
	if (!bIsSelected || !MeshComponent)
	{
		return;
	}

	static constexpr float TwoPi = 6.2831855f;
	const float StepSec = 1.f / FMath::Max(1.f, GemTune.SelectionUpdateHz);
	SelectionPhase += TwoPi * GemTune.SelectionPulseHz * StepSec;
	if (SelectionPhase > TwoPi)
	{
		SelectionPhase -= TwoPi;
	}

	const float s = FMath::Sin(SelectionPhase);
	const float c = FMath::Cos(SelectionPhase);
	MeshComponent->SetRelativeLocation(FVector(
		c * GemTune.SelectionWobbleXYUU,
		s * GemTune.SelectionWobbleXYUU,
		s * GemTune.SelectionBobAmplitudeUU));

	const float ScaleMul = 1.f + s * GemTune.SelectionScaleAmp;
	MeshComponent->SetRelativeScale3D(MeshBaseRelativeScale * ScaleMul);
}

void AMatch3Gem::StopSelectionAnimation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_SelectionIdle);
	}

	SelectionPhase = 0.f;
	if (MeshComponent)
	{
		MeshComponent->SetRelativeLocation(FVector::ZeroVector);
		MeshComponent->SetRelativeScale3D(MeshBaseRelativeScale);
	}
}

void AMatch3Gem::StartSelectionAnimationIfNeeded()
{
	if (!bIsSelected || !bSelectionPressActive || !MeshComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(TimerHandle_SelectionIdle);

	const float StepSec = 1.f / FMath::Max(1.f, GemTune.SelectionUpdateHz);

	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &AMatch3Gem::UpdateSelectionVisual);
	World->GetTimerManager().SetTimer(
		TimerHandle_SelectionIdle,
		Delegate,
		StepSec,
		true);
}

void AMatch3Gem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetSelected(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_SelectionIdle);
		World->GetTimerManager().ClearTimer(TimerHandle_SelectAnimation);
		World->GetTimerManager().ClearTimer(TimerHandle_MoveAnimation);
	}

	Super::EndPlay(EndPlayReason);
}

FLinearColor AMatch3Gem::ResolveGemColor(EGemType InType) const
{
	switch (InType)
	{
	case EGemType::Red:
		return GemTune.GemColorRed;
	case EGemType::Blue:
		return GemTune.GemColorBlue;
	case EGemType::Green:
		return GemTune.GemColorGreen;
	case EGemType::Yellow:
		return GemTune.GemColorYellow;
	default:
		return GemTune.GemColorEmpty;
	}
}

void AMatch3Gem::EnsureDynamicMaterial()
{
	if (GemMID)
	{
		return;
	}

	UMaterialInterface* Base = MeshComponent ? MeshComponent->GetMaterial(0) : nullptr;
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (MeshComponent && Base)
		{
			MeshComponent->SetMaterial(0, Base);
		}
	}

	if (Base && MeshComponent)
	{
		GemMID = UMaterialInstanceDynamic::Create(Base, MeshComponent);
		if (GemMID)
		{
			MeshComponent->SetMaterial(0, GemMID);
		}
	}
}

void AMatch3Gem::SetGemType(EGemType NewType)
{
	GemType = NewType;
	EnsureDynamicMaterial();

	if (GemMID)
	{
		ApplyGemTint(GemMID, ResolveGemColor(GemType));
	}
}

void AMatch3Gem::SetGridPosition(FIntPoint NewPosition)
{
	GridPosition = NewPosition;

#if WITH_EDITOR
	const FString NewName = FString::Printf(TEXT("Gem_%d_%d"), NewPosition.X, NewPosition.Y);
	SetActorLabel(NewName);
#endif
}

void AMatch3Gem::SetSelected(bool bSelected)
{
	if (bIsSelected == bSelected)
	{
		return;
	}

	bIsSelected = bSelected;
	Highlight(bSelected);

	if (bSelected)
	{
		if (MeshComponent)
		{
			MeshBaseRelativeScale = MeshComponent->GetRelativeScale3D();
		}
		SelectionPhase = 0.f;
		bSelectionPressActive = false;
		StopSelectionAnimation();
	}
	else
	{
		bSelectionPressActive = false;
		StopSelectionAnimation();
	}
}

void AMatch3Gem::SetSelectionPressActive(bool bPressActive)
{
	if (bSelectionPressActive == bPressActive)
	{
		return;
	}

	bSelectionPressActive = bPressActive;
	if (!bIsSelected || !MeshComponent)
	{
		return;
	}

	if (bSelectionPressActive)
	{
		SelectionPhase = 0.f;
		StartSelectionAnimationIfNeeded();
	}
	else
	{
		StopSelectionAnimation();
	}
}

void AMatch3Gem::SetMatched(bool bMatched)
{
	bIsMatched = bMatched;
	if (bMatched)
	{
		PlayMatchAnimation();
	}
}

void AMatch3Gem::Highlight(bool bEnable)
{
	if (!MeshComponent)
	{
		return;
	}

	if (bEnable)
	{
		MeshComponent->SetRenderCustomDepth(true);
		MeshComponent->SetCustomDepthStencilValue(static_cast<int32>(GemTune.CustomDepthStencilValue));
	}
	else
	{
		MeshComponent->SetRenderCustomDepth(false);
	}
}

void AMatch3Gem::PlaySelectAnimation()
{
	if (!MeshComponent)
	{
		return;
	}

	const FVector OriginalScale = GetActorScale3D();
	const FVector TargetScale = OriginalScale * GemTune.SelectAnimScaleMultiplier;
	SetActorScale3D(TargetScale);

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([this, OriginalScale]()
			{
				if (IsValid(this) && MeshComponent)
				{
					SetActorScale3D(OriginalScale);
				}
			});
		World->GetTimerManager().SetTimer(
			TimerHandle_SelectAnimation,
			TimerDelegate,
			GemTune.SelectAnimDurationSec,
			false);
	}
}

void AMatch3Gem::PlayMatchAnimation()
{
	bIsMoving = true;
	if (UWorld* World = GetWorld())
	{
		FTimerHandle Tmp;
		World->GetTimerManager().SetTimer(
			Tmp,
			[this]()
			{
				if (IsValid(this))
				{
					bIsMoving = false;
				}
			},
			GemTune.MatchBusyDurationSec,
			false);
	}
}

void AMatch3Gem::PlayMoveAnimation(FVector TargetPosition, float Duration)
{
	if (Duration <= 0.f || !MeshComponent)
	{
		return;
	}

	bIsMoving = true;
	MoveAnimElapsed = 0.f;

	const FVector StartPosition = GetActorLocation();
	const float StepTime = 1.f / FMath::Max(1.f, GemTune.MoveAnimTickHz);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_MoveAnimation);

		FTimerDelegate MoveDelegate;
		MoveDelegate.BindLambda([this, StartPosition, TargetPosition, Duration, StepTime]()
			{
				if (!IsValid(this))
				{
					return;
				}

				MoveAnimElapsed += StepTime;
				const float Alpha = FMath::Clamp(MoveAnimElapsed / Duration, 0.f, 1.f);
				SetActorLocation(FMath::Lerp(StartPosition, TargetPosition, Alpha));

				if (Alpha >= 1.f)
				{
					bIsMoving = false;
					MoveAnimElapsed = 0.f;
					GetWorldTimerManager().ClearTimer(TimerHandle_MoveAnimation);
				}
			});

		World->GetTimerManager().SetTimer(TimerHandle_MoveAnimation, MoveDelegate, StepTime, true);
	}
}
