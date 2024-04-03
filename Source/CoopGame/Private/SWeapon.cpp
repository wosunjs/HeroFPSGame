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


// ��������̨����debug
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

	// ���������鲥����Ƶ��
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
	// ׷�ٴ�pawn˫�۵�ʮ��׼��λ�õĹ켣

	// �ͻ��˴����Ŀ����������ServerFire()
	if(GetLocalRole() < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();	// ��ȡ������
	if(MyOwner)
	{
		// ��ȡ������˫������
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		// ����˫�����ݻ�ȡ�����λ��Ŀ���λ�ã�˫��λ�� + ˫����ת��ת����ʸ�� * 10000��
		FVector ShotDirection = EyeRotation.Vector();

		// ��ɨ�������½��������������
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);	//������ˮƽ�ʹ�ֱ�϶�����HalRad���

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);


		// ������ײ��ѯ����
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);	//����������
		QueryParams.AddIgnoredActor(this);		//��������
		QueryParams.bTraceComplex = true;		//����׷��Ϊ�棬ʹ׷�ٶ�λ��ȷ��Ŀ���������е�ÿ��������
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TraceEndPoint = TraceEnd;		// ����Ŀ�����

		FHitResult Hit;							// �洢�������塢�����Զ�����з��������

		// �ܻ�����д����,����������㲥��һ�ε��ŵ�Ч��
		EPhysicalSurface SurfaceType = SurfaceType_Default;

		// ���������������ײ�򴥷�Ч��
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams)) {
			// ���������裬�����˺�����
			AActor* HitActor = Hit.GetActor();

			// ��ȡ�����������
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());		// �������tmp��Ȼ�ǿ�

			float ActualDamage = BaseDamage;
			if(SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= 1.5f;
			}

			// ��ɵ��˺�
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			// �ŵ���Ч��
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint, ShotDirection);
			
			// ����Ŀ���滻Ϊ�ܻ���
			TraceEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)		// ��������켣(��ɫ������Ҫʼ�ճ��֣�����ʱ��1s)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEndPoint, FColor::White, false, 1.0f, 0, 1.0f);
		}

		PlayFireEffects(TraceEndPoint);

		// ����ڷ������˳ɹ������㲥�˴��ŵ���
		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.Tracefrom = ShotDirection;
			HitScanTrace.Traceto = TraceEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;		// ��¼��һ�ο���ʱ��
	}
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	// ���ö�ʱ��
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::PlayFireEffects(FVector TraceEndPoint)
{
	// ��ǹ�ڴ�����ǹ�ڻ�����Ч
	if (MuzzleEffect)
	{
		// ��������۴����Ż���Ч��
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	// ������û�д��ж���ǹ�ڴ���������Ч��
	if (TraceEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);		//ǹ��λ��
		UParticleSystemComponent* TraceComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
			TraceEffect, MuzzleLocation);
		if (TraceComp)
		{
			TraceComp->SetVectorParameter(TracerTargetName, TraceEndPoint);
		}
	}

	// ֻ�п�ǹ�Ŀͻ��˷������ڶ���
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

		// ���ŵ��㻭�����Ч��
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

// �ɿ�����֤����,����������׷��ؼ٣�������һ�����Ŀͻ��˻�ӷ������Ͽ�����
bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	// �㲥�������Ч��
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

	// �����������ڽ��ӵ��켣�����ݸ��Ƹ������ɸù켣��֮��Ŀͻ���
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}

