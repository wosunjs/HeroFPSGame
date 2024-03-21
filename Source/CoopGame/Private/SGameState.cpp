// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameState.h"
#include "Net/UnrealNetwork.h"

void ASGameState::OnRep_WaveState(EWaveState OldState)
{
	WaveStateChanged(WaveState, OldState);
}

void ASGameState::SetWaveState(EWaveState NewState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		EWaveState OldState = WaveState;

		WaveState = NewState;
		// Call on server
		OnRep_WaveState(OldState);
	}
}

void ASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 在生命周期内将子弹轨迹线数据复制给除生成该轨迹线之外的客户端
	DOREPLIFETIME(ASGameState, WaveState);
}