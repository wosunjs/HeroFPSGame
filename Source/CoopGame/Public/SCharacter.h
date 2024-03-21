// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"


//向前编译
class UCameraComponent;
class USpringArmComponent;
class USHealthComponent;
class ASWeapon;

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);	//向前移动的值(距离和方向)
	void MoveRight(float Value);	//向右移动的值(距离和方向)

	void BeginCrouch();		// 开始下蹲
	void EndCrouch();		// 结束下蹲

	// 标记为随处可见和仅蓝图可见：让该属性仅能通过在构造函数中创建实例
	// 而后将为其添加属性，而非直接输入一个全新的对象
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;			//相机组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;		//弹簧臂组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;			//生命值组件

	bool bWantsToZoom;		// 是否需要变焦推进

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100.0f))
	float ZoomInterpSpeed;	// 聚焦速度

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;		// 变焦后的FOV值
	float DefaultFOV;		// 游戏开始时设置默认变焦

	void BeginZoom();		// 开始聚焦
	void EndZoom();			// 结束聚焦

	void StartFire();
	void StopFire();

	UPROPERTY(Replicated)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;	// 为玩家生成武器类（设为ASWeapon的子类）

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;
	
	void Fire();

	//UFUNCTION(BlueprintCallable, Category = "Player")
	UFUNCTION()
	void OnHealthChange(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// 人形体是否死亡
	UPROPERTY(Replicated ,BlueprintReadOnly, Category = "Player")
	bool bDied;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 重载获取视野位置函数，使子弹能在合适的位置发出
	virtual FVector GetPawnViewLocation() const override;
};
