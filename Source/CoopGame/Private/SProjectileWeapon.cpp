// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"

void ASProjectileWeapon::Fire()
{
	// 追踪从pawn双眼到十字准星位置的轨迹

	AActor* MyOwner = GetOwner();	// 获取所有者
	if (MyOwner && ProjectileClass)
	{
		// 获取所有者双眼数据
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		// 获取枪口位置
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		// Actor生成参数,总是生成
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		// 创建发射物
		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
	}
}
