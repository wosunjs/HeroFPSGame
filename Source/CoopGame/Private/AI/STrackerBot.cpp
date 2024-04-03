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


// 创建控制台变量debug
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
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);	//组件关联函数

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);		//只对玩家产生碰撞
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
		// 初始化目标位置为自身，然后在每帧中调用寻路
		NextPathPoint = GetNextPathPoint();
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr) //如果材质未赋值就创建 并 设置新的动态实例
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of &s"), *FString::SanitizeFloat(Health), *GetName());		//将值改为数组进行记录

	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// 尝试获取玩家位置
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
		// 当长时间未到达目标点时我们便另寻他路
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0f, false);
		// 当长时间未到达目标点时我们直接自毁
		//GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 30.0f);
		

		// 如果当前点不是最近的路径点则获取下一个路径点
		if (NavPath && NavPath->PathPoints.Num() > 1) {
			// 返回下一个路径点
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

		if (DistanceToTarget > RequiredDistanceToTarget)	// 距离过大不断移向下一个目标
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
		else  // 距离小只需要更新目的地
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

	// 在原位产生爆炸的粒子效果
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	// 播放爆炸特效
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	// 爆炸后网格不可见、无视碰撞
	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if(GetLocalRole() == ROLE_Authority)
	{
		// 自身对伤害免疫
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);
		// 产生辐射伤害
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		// 绘制爆炸测试球体
		if (DebugTrackBotDrawing) 
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);
		}

		// 立刻删除actor
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
				// 如果与玩家重叠就开始自毁程序
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}
			
			bStartedSlefDestruction = true;

			// 当自毁触发时播放音效
			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}

}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}


