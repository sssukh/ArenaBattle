// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ABCharacterBase.h"
#include "Engine/StreamableManager.h"
#include "Interface/ABCharacterAIInterface.h"
#include "ABCharacterNonPlayer.generated.h"

/**
 * 
 */
// DefaultArenaBattle.ini를 사용하겠다는 뜻
UCLASS(config = ArenaBattle)
class ARENABATTLE_API AABCharacterNonPlayer : public AABCharacterBase, public IABCharacterAIInterface
{
	GENERATED_BODY()
public:
	AABCharacterNonPlayer();
protected:
	virtual void PostInitializeComponents() override;

protected:
	void SetDead()override;
	void NPCMeshLoadCompleted();

	// 프로젝트가 로딩될 때 자동으로 값들이 채워진다.
	UPROPERTY(config)
	TArray<FSoftObjectPath>	 NPCMeshes;

	TSharedPtr<FStreamableHandle> NPCMeshHandle;

	// AI Section
protected:
	virtual float GetAIPatrolRadius() override;
	virtual float GetAIDetectRange() override;
	virtual float GetAIAttackRange() override;
	virtual float GetAITurnSpeed() override;

	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) override;
	virtual void AttackByAI() override;

	FAICharacterAttackFinished OnAttackFinished;

	virtual void NotifyComboActionEnd() override;

};
