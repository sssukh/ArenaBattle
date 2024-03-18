// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ABUserWidget.h"
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
	// ������ �ʱ�ȭ�� �� HpProgressBar�� ���������� ��.
	// �� �Լ��� �Ҹ� ������ UI�� ���õ� ��� ��ɵ��� ���� �ʱ�ȭ�� �Ϸ�� �����̴�.
	virtual void NativeConstruct()override;
public:
	FORCEINLINE void SetMaxHp(float NewMaxHp) { MaxHp = NewMaxHp; }
	void UpdateHpBar(float NewCurrentHp);

protected:
	UPROPERTY()
	TObjectPtr<class UProgressBar>	HpProgressBar;

	UPROPERTY()
	float MaxHp;
};
