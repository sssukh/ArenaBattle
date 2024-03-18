// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ABCharacterStatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnHpZeroDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHpChangedDelegate, float /*CurrentHp*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENABATTLE_API UABCharacterStatComponent : public UActorComponent
{
	GENERATED_BODY()
public:	
	// Sets default values for this component's properties
	UABCharacterStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	FOnHpZeroDelegate OnHpZero;
	FOnHpChangedDelegate OnHpChanged;
	FORCEINLINE float GetMaxHp() { return MaxHp; }
	FORCEINLINE float GetCurrentHp() { return CurrentHp; }
	float ApplyDamage(float InDamage);


protected:
	// Hp 변경
	void SetHp(float NewHp);

	UPROPERTY(VisibleInstanceOnly,Category = Stat)
	float MaxHp;

	//  값들은 디스크에 저장되는데 Transient라는 매크로를 통해서 불필요한 저장을 막을수 있다.
	UPROPERTY(Transient, VisibleInstanceOnly, Category = Stat)
	float CurrentHp;

		
};
