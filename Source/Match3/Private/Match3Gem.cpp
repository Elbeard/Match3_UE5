#include "Match3Gem.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

namespace
{
	static constexpr float ClickRadiusUU = 48.f;
}

AMatch3Gem::AMatch3Gem()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ClickCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ClickCollision"));
	ClickCollision->SetupAttachment(MeshComponent);
	ClickCollision->SetSphereRadius(ClickRadiusUU);
	ClickCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// ������� 2D-�����: ���������� plane ������ sphere.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(PlaneAsset.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.f));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMat.Succeeded())
	{
		MeshComponent->SetMaterial(0, ShapeMat.Object);
	}
}

void AMatch3Gem::BeginPlay()
{
	Super::BeginPlay();
	// �������� ��������� ���� �� GemType ����� ������.
	SetGemType(GemType);
}

void AMatch3Gem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_SelectAnimation);
		World->GetTimerManager().ClearTimer(TimerHandle_MoveAnimation);
	}

	Super::EndPlay(EndPlayReason);
}

FLinearColor AMatch3Gem::ColorForGemType(EGemType InType)
{
	switch (InType)
	{
	case EGemType::Red:
		return FLinearColor(0.92f, 0.18f, 0.18f);
	case EGemType::Blue:
		return FLinearColor(0.18f, 0.45f, 0.95f);
	case EGemType::Green:
		return FLinearColor(0.22f, 0.82f, 0.32f);
	case EGemType::Yellow:
		return FLinearColor(0.95f, 0.86f, 0.20f);
	default:
		return FLinearColor(0.65f, 0.65f, 0.65f);
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
		// � �������� �������� �������� BaseColor.
		static const FName BaseColorName(TEXT("BaseColor"));
		GemMID->SetVectorParameterValue(BaseColorName, ColorForGemType(GemType));
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
	if (bIsSelected != bSelected)
	{
		bIsSelected = bSelected;
		Highlight(bSelected);
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
		MeshComponent->SetCustomDepthStencilValue(1);
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

	// ���������� feedback: ������� scale up � ������� �� �������.
	const FVector OriginalScale = GetActorScale3D();
	const FVector TargetScale = OriginalScale * 1.12f;
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
		World->GetTimerManager().SetTimer(TimerHandle_SelectAnimation, TimerDelegate, 0.08f, false);
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
			0.05f,
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
	const float StepTime = 1.f / 60.f;

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
