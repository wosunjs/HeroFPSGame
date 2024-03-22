// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"

void ASProjectileWeapon::Fire()
{
	// ׷�ٴ�pawn˫�۵�ʮ��׼��λ�õĹ켣

	AActor* MyOwner = GetOwner();	// ��ȡ������
	if (MyOwner && ProjectileClass)
	{
		// ��ȡ������˫������
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		// ��ȡǹ��λ��
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		// Actor���ɲ���,��������
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		// ����������
		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
	}
}
