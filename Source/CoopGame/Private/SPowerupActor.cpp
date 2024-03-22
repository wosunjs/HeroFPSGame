// Fill out your copyright notice in the Description page of Project Settings.


#include "SPowerupActor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerupActor::ASPowerupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PowerupInterval = 0.0f;
	TotalNrOfTicks = 0;

	bIsPowerupActive = false;

	SetReplicates(true);
}


void ASPowerupActor::OnTickPowerup()
{
	++TicksProcessed;

	OnPowerupTicked();

	// 如果是最后一次更新，调用OnExpired()和清除计时器
	if(TotalNrOfTicks <= TicksProcessed)
	{
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive();

		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}
}

void ASPowerupActor::OnRep_PowerupActive()
{
	OnPowerupActiveChanged(bIsPowerupActive);
}

void ASPowerupActor::ActivatePowerup(AActor* ActiveFor)
{
	OnActivated(ActiveFor);

	bIsPowerupActive = true;
	OnRep_PowerupActive();

	// 间隔性生成效果
	if (PowerupInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true);
	}
	else  // 一次性效果
	{
		OnTickPowerup();
	}
}

// 对复制变量的条件做出修饰
void ASPowerupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 在生命周期内复制标记为复制的变量
	DOREPLIFETIME(ASPowerupActor, bIsPowerupActive);
}

