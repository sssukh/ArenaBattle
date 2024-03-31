// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABCharacterControlData.h"
#include "Animation/AnimMontage.h"
#include "ABComboActionData.h"
#include "Physics/ABCollision.h"
#include "Engine/DamageEvents.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "UI/ABWidgetComponent.h"
#include "UI/ABHpBarWidget.h"
#include "Item/ABWeaponItemData.h"

DEFINE_LOG_CATEGORY(LogABCharacter);

// Sets default values
AABCharacterBase::AABCharacterBase()
{
 	// Pawn
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionProfileName(CPROFILE_ABCAPSULE);

    // Movement
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 700.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.0f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

    // Mesh
    GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.0f), FRotator(0.0f, -90.0f, 0.0f));
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard'"));
    if (CharacterMeshRef.Object)
    {
        GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
    }

    static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceMeshRef(TEXT("/Game/ArenaBattle/Animation/ABP_ABCharacter.ABP_ABCharacter_C"));
    if (AnimInstanceMeshRef.Class)
    {
        GetMesh()->SetAnimInstanceClass(AnimInstanceMeshRef.Class);
    }

    static ConstructorHelpers::FObjectFinder<UABCharacterControlData> QuaterDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Quater.ABC_Quater'"));
    if (QuaterDataRef.Object)
    {
        CharacterControlManager.Add(ECharacterControlType::Quater, QuaterDataRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UABCharacterControlData> ShoulderDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Shoulder.ABC_Shoulder'"));
    if (ShoulderDataRef.Object)
    {
        CharacterControlManager.Add(ECharacterControlType::Shoulder, ShoulderDataRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UABComboActionData> ComboActionDataRef(TEXT("/Script/ArenaBattle.ABComboActionData'/Game/ArenaBattle/CharacterAction/ABA_ComboAttack.ABA_ComboAttack'"));
    if (ComboActionDataRef.Object)
    {
        ComboActionData = ComboActionDataRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboActionMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_ComboAttack.AM_ComboAttack'"));
    if (ComboActionMontageRef.Object)
    {
        ComboActionMontage = ComboActionMontageRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> DeadMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_Dead.AM_Dead'"));
    if (DeadMontageRef.Object)
    {
        DeadMontage = DeadMontageRef.Object;
    }

    // Stat Component
    Stat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("Stat"));


    // Widget Component
    HpBar = CreateDefaultSubobject<UABWidgetComponent>(TEXT("HpBar"));
    HpBar->SetupAttachment(GetMesh());
    HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.f));
    static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/ArenaBattle/UI/WBP_HPBar.WBP_HPBar_C"));
    if (HpBarWidgetRef.Class)
    {
        HpBar->SetWidgetClass(HpBarWidgetRef.Class);
        // 위젯 공간 2D, 
        HpBar->SetWidgetSpace(EWidgetSpace::Screen);
        // 위젯이 담길 캔버스의 작업공간 크기
        HpBar->SetDrawSize(FVector2D(150.0f, 15.0f));
        HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Item Actions
    TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::EquipWeapon)));
    TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::DrinkPotion)));
    TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::ReadScroll)));

    // Weapon Component
    Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
    Weapon->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));
}

void AABCharacterBase::BeginPlay()
{
    Super::BeginPlay();
    Stat->OnHpZero.AddUObject(this, &AABCharacterBase::SetDead);

}



void AABCharacterBase::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
    // Pawn
    bUseControllerRotationYaw = CharacterControlData->bUseControllerRotationYaw;

    // CharacterMovement
    GetCharacterMovement()->bOrientRotationToMovement = CharacterControlData->bOrientRotationToMovement;
    GetCharacterMovement()->bUseControllerDesiredRotation = CharacterControlData->bUseControllerDesiredRotation;
    GetCharacterMovement()->RotationRate = CharacterControlData->RotationRate;
}

void AABCharacterBase::ProcessComboCommand()
{
    if (CurrentCombo == 0)
    {
        ComboActionBegin();
        return;
    }

    // ComboEffectiveTime이 지나면 ComboCheck에서 타이머를 초기화시킨다.
    // 타이머가 유효하면 ComboCheck()하기 이전에 커맨드가 발동했다는 뜻이다.
    if (!ComboTimerHandle.IsValid())
    {
        HasNextComboCommand = false;
    }
    else
    {
        HasNextComboCommand = true;
    }
}

void AABCharacterBase::ComboActionBegin()
{
    // Combo Status
    CurrentCombo = 1;

    // Movement Setting
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

    //Animation Setting
    const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    AnimInstance->Montage_Play(ComboActionMontage, AttackSpeedRate);

    // EndDelegate에 ComboActionEnd를 바인드시킨다.
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AABCharacterBase::ComboActionEnd);
    
    // 종료가 안되면 호출이 안된다? -> 결국 마지막 콤보에서는 종료가 되게 되있다.
    // ComboActio Montage가 종료가 될 때 EndDelegate호출 -> ComboActionEnd가 호출된다. 
    AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboActionMontage);

    // 시간 초기화
    ComboTimerHandle.Invalidate();
    SetComboCheckTimer();
}

void AABCharacterBase::ComboActionEnd(UAnimMontage* TargetMontage, bool IsProperlyEnded)
{
    ensure(CurrentCombo != 0);
    CurrentCombo = 0;
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

    NotifyComboActionEnd();
}

void AABCharacterBase::NotifyComboActionEnd()
{
}

void AABCharacterBase::SetComboCheckTimer()
{
    int32 ComboIndex = CurrentCombo - 1;
    ensure(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

    const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
    float ComboEffectiveTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;
    // 마지막 Combo의 EffectiveFrameCount가 -1 이기 때문에 마지막 콤보가 실행되고 나서는 넘어가지 않는다
    if (ComboEffectiveTime > 0.0f)
    {
        // ComboTimerhandle을 활성화하고 ComboEffectiveTime이 지나면 ComboCheck()함수를 호출하지만 한번만 호출
        GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle,this,&AABCharacterBase::ComboCheck,ComboEffectiveTime,false);
    }
}

void AABCharacterBase::ComboCheck()
{
    // Timer 초기화
    ComboTimerHandle.Invalidate();

    // 다음 콤보 커맨드가 들어오면
    if (HasNextComboCommand)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        // ComboCount를 더해주지만 MaxComboCount를 넘지않도록 Clamp시켜준다
        CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
        // 섹션이름 가져오기 prefix인 ComboAction + CurrentCombo
        FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
        // 입력한 지점으로 넘어가서 재생한다.
        AnimInstance->Montage_JumpToSection(NextSection, ComboActionMontage);
        SetComboCheckTimer();
        HasNextComboCommand = false;
    }
}

// 트레이스 채널을 활용해서 물체가 서로 충돌되는지 검사하는 로직
void AABCharacterBase::AttackHitCheck()
{
    FHitResult OutHitResult;
    // 첫번째 인자는 InTraceTag라고 나중에 이 Collision을 어떤 태그정보로 분석할 때 식별자 정보로 사용된다.
    // SCENE_QUERY_STAT은 언리얼에서 지원하는 분석 툴이 있다. 여기서 Attack이라는 태그로 우리가 수행한 작업에 대해서 조사할 수 있게 태그를 추가하는 것이다.
    // 두번째 인자는 bInTraceComplex로 복잡한 형태의 충돌체(캡슐이나 구 가 아닌 복잡한 것들)도 감지할지에 대한 옵션
    // 세번째 인자는 InIgnoreActor로 무시할 액터(여기서는 자기 자신만 무시하면 된다.)
    FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

    const float AttackRange = Stat->GetTotalStat().AttackRange;
    const float AttackRadius = Stat->GetAttackRadius();
    const float AttackDamage = Stat->GetTotalStat().Attack;
    // 투사의 시작지점
    const FVector Start = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
    // 투사의 끝지점
    const FVector End = Start + GetActorForwardVector() * AttackRange;

    // 월드가 제공하기 때문에 GetWorld()로 포인터 받아옴
    // 결과값을 받아올 수 있는 구조체 FHitResult를 넣어줌
    bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);
    if (HitDetected)
    {
        FDamageEvent DamageEvent;
        OutHitResult.GetActor()->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
    }
#if ENABLE_DRAW_DEBUG

    FVector CapsuleOrigin = Start + (End - Start) * 0.5f;
    float CapsuleHalfHeight = AttackRange * 0.5f;
    FColor DrawColor = HitDetected ? FColor::Green : FColor::Red;

    DrawDebugCapsule(GetWorld(), CapsuleOrigin, CapsuleHalfHeight, AttackRadius, FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat(), DrawColor, false, 5.0f);

#endif
}

float AABCharacterBase::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Instigator는 피해를 입힌 가해자, DamageCause는 가해자가 사용한 무기나 가해자가 빙의한 폰
    // 이를 이용해서 내가 누구에게서 데미지를 받았는지 파악할 수 있다.
    Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);


    Stat->ApplyDamage(Damage);

    return Damage;
}

void AABCharacterBase::SetDead()
{
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
    PlayDeadAnimation();
    // Collision 기능 끄기
    SetActorEnableCollision(false);
    // HpBar 안보이게 설정
    HpBar->SetHiddenInGame(true);
}

void AABCharacterBase::PlayDeadAnimation()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    // 모든 몽타주 재생 중지
    AnimInstance->StopAllMontages(0.0f);
    AnimInstance->Montage_Play(DeadMontage, 1.0f);
}

void AABCharacterBase::SetupCharacterWidget(UABUserWidget* InUserWidget)
{
    UABHpBarWidget* HpBarWidget = Cast<UABHpBarWidget>(InUserWidget);
    if (HpBarWidget)
    {
        HpBarWidget->SetMaxHp(Stat->GetTotalStat().MaxHp);
        HpBarWidget->UpdateHpBar(Stat->GetCurrentHp());
        Stat->OnHpChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateHpBar);
    }
}

void AABCharacterBase::TakeItem(class UABItemData* InItemData)
{
    if (InItemData)
    {
        TakeItemActions[(uint8)InItemData->Type].ItemDelegate.ExecuteIfBound(InItemData);
    }
}

void AABCharacterBase::DrinkPotion(UABItemData* InItemData)
{
    UE_LOG(LogABCharacter, Log, TEXT("Drink Potion"));
}

void AABCharacterBase::ReadScroll(UABItemData* InItemData)
{
    UE_LOG(LogABCharacter, Log, TEXT("Read Scroll"));

}

void AABCharacterBase::EquipWeapon(UABItemData* InItemData)
{
    UABWeaponItemData* WeaponItemData = Cast<UABWeaponItemData>(InItemData);
    if (InItemData)
    {
        if (WeaponItemData->WeaponMesh.IsPending())
        {
            WeaponItemData->WeaponMesh.LoadSynchronous();
        }


        Weapon->SetSkeletalMesh(WeaponItemData->WeaponMesh.Get());

        Stat->SetModifierStat(WeaponItemData->ModifierStat);

    }

}

int32 AABCharacterBase::GetLevel()
{
    return Stat->GetCurrentLevel();
}

void AABCharacterBase::SetLevel(int32 InNewLevel)
{
    Stat->SetLevelStat(InNewLevel);
}
