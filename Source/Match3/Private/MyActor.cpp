// Укажите уведомление об авторских правах на странице Description в настройках проекта.


#include "MyActor.h"

// Значения по умолчанию
AMyActor::AMyActor()
{
 	// Включить вызов Tick() каждый кадр (можно отключить ради производительности, если не нужно)
	PrimaryActorTick.bCanEverTick = true;

}

// Вызывается при старте игры или при появлении актора в мире
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Вызывается каждый кадр
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
