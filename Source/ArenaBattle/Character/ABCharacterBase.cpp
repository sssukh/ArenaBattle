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
        // ���� ���� 2D, 
        HpBar->SetWidgetSpace(EWidgetSpace::Screen);
        // ������ ��� ĵ������ �۾����� ũ��
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

    // ComboEffectiveTime�� ������ ComboCheck���� Ÿ�̸Ӹ� �ʱ�ȭ��Ų��.
    // Ÿ�̸Ӱ� ��ȿ�ϸ� ComboCheck()�ϱ� ������ Ŀ�ǵ尡 �ߵ��ߴٴ� ���̴�.
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

    // EndDelegate�� ComboActionEnd�� ���ε��Ų��.
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AABCharacterBase::ComboActionEnd);
    
    // ���ᰡ �ȵǸ� ȣ���� �ȵȴ�? -> �ᱹ ������ �޺������� ���ᰡ �ǰ� ���ִ�.
    // ComboActio Montage�� ���ᰡ �� �� EndDelegateȣ�� -> ComboActionEnd�� ȣ��ȴ�. 
    AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboActionMontage);

    // �ð� �ʱ�ȭ
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
    // ������ Combo�� EffectiveFrameCount�� -1 �̱� ������ ������ �޺��� ����ǰ� ������ �Ѿ�� �ʴ´�
    if (ComboEffectiveTime > 0.0f)
    {
        // ComboTimerhandle�� Ȱ��ȭ�ϰ� ComboEffectiveTime�� ������ ComboCheck()�Լ��� ȣ�������� �ѹ��� ȣ��
        GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle,this,&AABCharacterBase::ComboCheck,ComboEffectiveTime,false);
    }
}

void AABCharacterBase::ComboCheck()
{
    // Timer �ʱ�ȭ
    ComboTimerHandle.Invalidate();

    // ���� �޺� Ŀ�ǵ尡 ������
    if (HasNextComboCommand)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

        // ComboCount�� ���������� MaxComboCount�� �����ʵ��� Clamp�����ش�
        CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
        // �����̸� �������� prefix�� ComboAction + CurrentCombo
        FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
        // �Է��� �������� �Ѿ�� ����Ѵ�.
        AnimInstance->Montage_JumpToSection(NextSection, ComboActionMontage);
        SetComboCheckTimer();
        HasNextComboCommand = false;
    }
}

// Ʈ���̽� ä���� Ȱ���ؼ� ��ü�� ���� �浹�Ǵ��� �˻��ϴ� ����
void AABCharacterBase::AttackHitCheck()
{
    FHitResult OutHitResult;
    // ù��° ���ڴ� InTraceTag��� ���߿� �� Collision�� � �±������� �м��� �� �ĺ��� ������ ���ȴ�.
    // SCENE_QUERY_STAT�� �𸮾󿡼� �����ϴ� �м� ���� �ִ�. ���⼭ Attack�̶�� �±׷� �츮�� ������ �۾��� ���ؼ� ������ �� �ְ� �±׸� �߰��ϴ� ���̴�.
    // �ι�° ���ڴ� bInTraceComplex�� ������ ������ �浹ü(ĸ���̳� �� �� �ƴ� ������ �͵�)�� ���������� ���� �ɼ�
    // ����° ���ڴ� InIgnoreActor�� ������ ����(���⼭�� �ڱ� �ڽŸ� �����ϸ� �ȴ�.)
    FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

    const float AttackRange = Stat->GetTotalStat().AttackRange;
    const float AttackRadius = Stat->GetAttackRadius();
    const float AttackDamage = Stat->GetTotalStat().Attack;
    // ������ ��������
    const FVector Start = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
    // ������ ������
    const FVector End = Start + GetActorForwardVector() * AttackRange;

    // ���尡 �����ϱ� ������ GetWorld()�� ������ �޾ƿ�
    // ������� �޾ƿ� �� �ִ� ����ü FHitResult�� �־���
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
    // Instigator�� ���ظ� ���� ������, DamageCause�� �����ڰ� ����� ���⳪ �����ڰ� ������ ��
    // �̸� �̿��ؼ� ���� �������Լ� �������� �޾Ҵ��� �ľ��� �� �ִ�.
    Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);


    Stat->ApplyDamage(Damage);

    return Damage;
}

void AABCharacterBase::SetDead()
{
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
    PlayDeadAnimation();
    // Collision ��� ����
    SetActorEnableCollision(false);
    // HpBar �Ⱥ��̰� ����
    HpBar->SetHiddenInGame(true);
}

void AABCharacterBase::PlayDeadAnimation()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    // ��� ��Ÿ�� ��� ����
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
