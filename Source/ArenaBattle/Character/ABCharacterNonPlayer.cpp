// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterNonPlayer.h"

AABCharacterNonPlayer::AABCharacterNonPlayer()
{
}

void AABCharacterNonPlayer::SetDead()
{
	Super::SetDead();

	FTimerHandle DeadTimerHandle;
	// Destroy() 라는 내용을 가진 람다 함수를 만들어서 델리게이트에 부착시켜 SetTimer에 연결
	GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda(
		[&]()
		{
			Destroy();
		}

	), DeadEventDelayTime, false);
}
