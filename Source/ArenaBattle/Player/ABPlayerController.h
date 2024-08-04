// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ABPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ARENABATTLE_API AABPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AABPlayerController();

	// K2�� �ٴ� ������ �������Ʈ�� ������ Kismet�� ��Ÿ���� ���λ�� ������
	// GameScoreChanged�� ���� ������ ������ �̸��� �޶���ϱ� ����
	UFUNCTION(BlueprintImplementableEvent, Category = Game, Meta = (DisplayName = "OnScoreChangedCpp"))
	void K2_OnScoreChanged(int32 NeScore);

	UFUNCTION(BlueprintImplementableEvent, Category = Game, Meta = (DisplayName = "OnScoreClearCpp"))
	void K2_OnGameClear();	

	UFUNCTION(BlueprintImplementableEvent, Category = Game, Meta = (DisplayName = "OnScoreOverCpp"))
	void K2_OnGameOver();

	void GameScoreChanged(int32 NewScore);
	void GameClear();
	void GameOver();

protected:
	virtual void BeginPlay() override;

	//HUD Section
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
		TSubclassOf<class UABHUDWidget> ABHUDWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
		TObjectPtr<class UABHUDWidget> ABHUDWidget;
};