// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame/CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"


// 创建控制台变量debug
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Line"), 
	ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;

	RateOfFire = 600.0f;

	BulletSpread = 2.0f;

	SetReplicates(true);

	// 设置网络组播更新频率
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60.0f / RateOfFire;
	
}

void ASWeapon::Fire()
{
	// 追踪从pawn双眼到十字准星位置的轨迹

	// 客户端触发的开火函数会调用ServerFire()
	if(GetLocalRole() < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();	// 获取所有者
	if(MyOwner)
	{
		// 获取所有者双眼数据
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		// 根据双眼数据获取射击方位和目标点位置（双眼位置 + 双眼旋转量转方向矢量 * 10000）
		FVector ShotDirection = EyeRotation.Vector();

		// 在扫射的情况下进行射击方向的随机
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);	//让其在水平和垂直上都进行HalRad随机

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);


		// 设置碰撞查询参数
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);	//忽略所有者
		QueryParams.AddIgnoredActor(this);		//忽略武器
		QueryParams.bTraceComplex = true;		//复合追踪为真，使追踪定位精确到目标网格体中的每块三角形
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TraceEndPoint = TraceEnd;		// 粒子目标参数

		FHitResult Hit;							// 存储击中物体、距离多远、击中方向等数据

		// 受击才重写材质,避免服务器广播上一次的着弹效果
		EPhysicalSurface SurfaceType = SurfaceType_Default;

		// 轨道上有物体则碰撞则触发效果
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams)) {
			// 如果射击受阻，进行伤害处理
			AActor* HitActor = Hit.GetActor();

			// 获取物体表面类型
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());		// 射击受阻tmp必然非空

			float ActualDamage = BaseDamage;
			if(SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 1.5f;
			}

			// 造成点伤害
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			// 着弹点效果
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint, ShotDirection);
			
			// 粒子目标替换为受击点
			TraceEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)		// 画出射击轨迹(白色，不需要始终出现，持续时间1s)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEndPoint, FColor::White, false, 1.0f, 0, 1.0f);
		}

		PlayFireEffects(TraceEndPoint);

		// 如果在服务器端成功开火便广播此次着弹点
		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.Tracefrom = ShotDirection;
			HitScanTrace.Traceto = TraceEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;		// 记录上一次开火时间
	}
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	// 设置定时器
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::PlayFireEffects(FVector TraceEndPoint)
{
	// 在枪口处画出枪口火焰特效
	if (MuzzleEffect)
	{
		// 发射器插槽处附着火焰效果
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	// 不管有没有打中都在枪口处生成烟雾效果
	if (TraceEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);		//枪口位置
		UParticleSystemComponent* TraceComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
			TraceEffect, MuzzleLocation);
		if (TraceComp)
		{
			TraceComp->SetVectorParameter(TracerTargetName, TraceEndPoint);
		}
	}

	// 只有开枪的客户端发生窗口抖动
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if(MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if(PC)
		{
			PC->ClientStartCameraShake(FireCameraShake);
		}
	}
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint, FVector TraceFrom)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector ShotDirection = ImpactPoint - TraceFrom;
		ShotDirection.Normalize();

		// 在着弹点画出冲击效果
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

// 可靠性验证函数,如果发现作弊返回假，调用这一函数的客户端会从服务器断开连接
bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	// 广播播放外观效果
	PlayFireEffects(HitScanTrace.Traceto);

	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.Traceto, HitScanTrace.Tracefrom);
}


// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 在生命周期内将子弹轨迹线数据复制给除生成该轨迹线之外的客户端
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}

