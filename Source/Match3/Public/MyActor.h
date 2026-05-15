// Укажите уведомление об авторских правах на странице Description в настройках проекта.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class MATCH3_API AMyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Значения по умолчанию для свойств актора
	AMyActor();

protected:
	// Вызывается при старте игры или при появлении актора в мире
	virtual void BeginPlay() override;

public:	
	// Вызывается каждый кадр
	virtual void Tick(float DeltaTime) override;

};
