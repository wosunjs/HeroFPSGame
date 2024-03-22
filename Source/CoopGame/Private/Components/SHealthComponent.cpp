// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "SGameMode.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	DefaultHealth = 100.0f;

	bIsDead = false;

	TeamNum = 255;

	IsFriendlyDamage = false;

	//SetIsReplicated(true);		//???
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 只有服务器能处理伤害
	if(GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}

	Health = DefaultHealth;
}

// 会将变化之前的生命值作为参数OldHealth广播
void USHealthComponent::OnRep_Health(float OldHealth)
{	
	// 在同步生命值的同时广播生命值发生变化时的效果
	float Damage = Health - OldHealth;

	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}

	if (IsFriendly(DamagedActor, DamageCauser) && DamagedActor != DamageCauser && IsFriendlyDamage == false) {
		return;
	}

	// 受限地更新生命值
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));

	bIsDead = Health <= 0.0f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if (bIsDead)
	{
		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

float USHealthComponent::GetHealth() const
{

	return Health;
}

void USHealthComponent::Heal(float HealAmount)
{
	if(HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount));

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);
}

bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (ActorA == nullptr || ActorB == nullptr) {
		return true;
	}
	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (HealthCompA == nullptr || HealthCompB == nullptr) {
		return true;
	}

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 在生命周期内将子弹轨迹线数据复制给除生成该轨迹线之外的客户端
	DOREPLIFETIME(USHealthComponent, Health);
}