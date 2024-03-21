// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"


//��ǰ����
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

	void MoveForward(float Value);	//��ǰ�ƶ���ֵ(����ͷ���)
	void MoveRight(float Value);	//�����ƶ���ֵ(����ͷ���)

	void BeginCrouch();		// ��ʼ�¶�
	void EndCrouch();		// �����¶�

	// ���Ϊ�洦�ɼ��ͽ���ͼ�ɼ����ø����Խ���ͨ���ڹ��캯���д���ʵ��
	// ����Ϊ��������ԣ�����ֱ������һ��ȫ�µĶ���
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;			//������

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;		//���ɱ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;			//����ֵ���

	bool bWantsToZoom;		// �Ƿ���Ҫ�佹�ƽ�

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100.0f))
	float ZoomInterpSpeed;	// �۽��ٶ�

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV;		// �佹���FOVֵ
	float DefaultFOV;		// ��Ϸ��ʼʱ����Ĭ�ϱ佹

	void BeginZoom();		// ��ʼ�۽�
	void EndZoom();			// �����۽�

	void StartFire();
	void StopFire();

	UPROPERTY(Replicated)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;	// Ϊ������������ࣨ��ΪASWeapon�����ࣩ

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;
	
	void Fire();

	//UFUNCTION(BlueprintCallable, Category = "Player")
	UFUNCTION()
	void OnHealthChange(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// �������Ƿ�����
	UPROPERTY(Replicated ,BlueprintReadOnly, Category = "Player")
	bool bDied;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ���ػ�ȡ��Ұλ�ú�����ʹ�ӵ����ں��ʵ�λ�÷���
	virtual FVector GetPawnViewLocation() const override;
};
