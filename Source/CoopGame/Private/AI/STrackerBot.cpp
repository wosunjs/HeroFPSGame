// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SHealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "SCharacter.h"
#include "Sound/SoundCue.h"


// ��������̨����debug
static int32 DebugTrackBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackBotDrawing(
	TEXT("COOP.DebugTrackBot"),
	DebugTrackBotDrawing,
	TEXT("Draw Debug Line for TrackBot"),
	ECVF_Cheat);

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);	//�����������

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);		//ֻ����Ҳ�����ײ
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	bUseVelocityChange = true;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionRadius = 350;
	ExplosionDamage = 45;

	SelfDamageInterval = 0.25;

}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if(GetLocalRole() == ROLE_Authority)
	{
		// ��ʼ��Ŀ��λ��Ϊ����Ȼ����ÿ֡�е���Ѱ·
		NextPathPoint = GetNextPathPoint();
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr) //�������δ��ֵ�ʹ��� �� �����µĶ�̬ʵ��
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of &s"), *FString::SanitizeFloat(Health), *GetName());		//��ֵ��Ϊ������м�¼

	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// ���Ի�ȡ���λ��
	AActor* BestTarget = nullptr;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		float NearDistance = FLT_MAX;
		if (TestPawn == nullptr || USHealthComponent::IsFriendly(TestPawn, this))
		{
			continue;
		}

		USHealthComponent* TestHealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			float Distance = (TestPawn->GetActorLocation() - this->GetActorLocation()).Size();
			if (Distance < NearDistance) 
			{
				Distance = NearDistance;
				BestTarget = TestPawn;
			}
		}
	} 

	if (BestTarget) 
	{
		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
		// ����ʱ��δ����Ŀ���ʱ���Ǳ���Ѱ��·
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0f, false);
		// ����ʱ��δ����Ŀ���ʱ����ֱ���Ի�
		//GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 30.0f);
		

		// �����ǰ�㲻�������·�������ȡ��һ��·����
		if (NavPath && NavPath->PathPoints.Num() > 1) {
			// ������һ��·����
			return NavPath->PathPoints[1];
		}
	}

	return GetActorLocation();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget > RequiredDistanceToTarget)	// ������󲻶�������һ��Ŀ��
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
			
			if (DebugTrackBotDrawing) 
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
			}
		}
		else  // ����Сֻ��Ҫ����Ŀ�ĵ�
		{
			NextPathPoint = GetNextPathPoint();

			if (DebugTrackBotDrawing) 
			{
				DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!");
			}
		}

		if (DebugTrackBotDrawing) 
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 2.0f, 1.0f);
		}
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	// ��ԭλ������ը������Ч��
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	// ���ű�ը��Ч
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	// ��ը�����񲻿ɼ���������ײ
	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if(GetLocalRole() == ROLE_Authority)
	{
		// ������˺�����
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);
		// ���������˺�
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		// ���Ʊ�ը��������
		if (DebugTrackBotDrawing) 
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);
		}

		// ����ɾ��actor
		SetLifeSpan(2.0f);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSlefDestruction && !bExploded)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && !USHealthComponent::IsFriendly(OtherActor, this))
		{
			if (GetLocalRole() == ROLE_Authority)
			{
				// ���������ص��Ϳ�ʼ�Իٳ���
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}
			
			bStartedSlefDestruction = true;

			// ���Իٴ���ʱ������Ч
			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}

}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}


