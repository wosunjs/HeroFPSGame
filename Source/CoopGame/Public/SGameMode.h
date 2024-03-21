// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"


enum class EWaveState : uint8;

// �����Զ��嶯̬_�ಥ_����_�������¼�����ΪFOnActorKilled
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	FTimerHandle TimerHandle_BotSpawner;

	FTimerHandle TimerHandle_NextWaveStart;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;

	// ��ǰ�������ɻ�������
	int32 NrOfBotsToSpawn;

	int32 WaveCount;

protected:
	// ����BP�����ɵ���������
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();

	// ��ʼ���ɻ�����
	void StartWave();

	// ֹͣ���ɻ�����
	void EndWave();

	// ������һ�����ɶ�ʱ��
	void PrepareForNextWave();

	// ��鵱ǰ�Ƿ���Ҫ������һ��������
	void CheckWaveState();

	// ����Ƿ�����Ҵ��
	void CheckAnyPlayerAlive();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void RestartDeadPlayers();

public:

	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "GameMode")
	FOnActorKilled OnActorKilled;


};
