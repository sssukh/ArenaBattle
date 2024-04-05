// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ABUserWidget.h"
#include "GameData/ABCharacterStat.h"
#include "ABHpBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class ARENABATTLE_API UABHpBarWidget : public UABUserWidget
{
	GENERATED_BODY()
public:
	UABHpBarWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// 위젯이 초기화될 때 HpProgressBar를 가져오도록 함.
	// 이 함수가 불릴 때에는 UI에 관련된 모든 기능들이 거의 초기화가 완료된 시점이다.
	virtual void NativeConstruct()override;
public:
	void UpdateStat(const FABCharacterStat& BaseStat, const FABCharacterStat& ModifierStat);
	void UpdateHpBar(float NewCurrentHp);
	FString GetHpStatText();

protected:
	UPROPERTY()
	TObjectPtr<class UProgressBar>	HpProgressBar;

	UPROPERTY()
	TObjectPtr<class UTextBlock>	HpStat;

	UPROPERTY()
	float CurrentHp;

	UPROPERTY()
	float MaxHp;
};
