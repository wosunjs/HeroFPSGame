// Fill out your copyright notice in the Description page of Project Settings.


#include "SPickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "SPowerupActor.h"
#include "TimerManager.h"

// Sets default values
ASPickupActor::ASPickupActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(75.0f);
	RootComponent = SphereComp;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	DecalComp->DecalSize = FVector(64, 75, 75);
	DecalComp->SetupAttachment(RootComponent);

	CooldownDuration = 10.0f;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASPickupActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetLocalRole() == ROLE_Authority)
	{
		Respawn();
	}
}

void ASPickupActor::Respawn()
{
	if (PowerUpClass == nullptr) 
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerUpClass是%s中的空指针，请更新你的蓝图"), *GetName());
		return;
	}

	// 配置类生成参数
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;	//总是生成，不检查碰撞

	// 生成增强效果实例
	PowerUpInstance = GetWorld()->SpawnActor<ASPowerupActor>(PowerUpClass, GetTransform(), SpawnParams);


}

void ASPickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(GetLocalRole() == ROLE_Authority && PowerUpInstance)
	{
		PowerUpInstance->ActivatePowerup(OtherActor);
		PowerUpInstance = nullptr;

		// 生成定时器用于重新生成
		GetWorldTimerManager().SetTimer(TimerHandle_RespawnTimer, this, &ASPickupActor::Respawn, CooldownDuration);
	}

}


