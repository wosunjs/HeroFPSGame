// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class COOPGAME_API ASProjectileWeapon : public ASWeapon
{
	GENERATED_BODY()
	
protected:

	virtual void Fire() override;

	// 从AActor生成一个投射物子类
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> ProjectileClass;
};
