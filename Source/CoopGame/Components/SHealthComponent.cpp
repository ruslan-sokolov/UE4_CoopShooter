// Fill out your copyright notice in the Description page of Project Settings.


#include "SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

#include "../CoopGameGameModeBase.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	DefaultHealth = 100.0f;

	bIsDead = false;

	SetIsReplicated(true);
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only hook if we are server
	if (GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}
	
	Health = DefaultHealth;

}


void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, 
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{

	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}

	// Update health clamped
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	// UE_LOG(LogTemp, Warning, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));

	bIsDead = Health <= 0.0f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
	OnHealthChangedClient.Broadcast(); // client event broadcast for server player too (currenly used for hp in hud)
	
	if (bIsDead)
	{
		ACoopGameGameModeBase* GM = GetWorld()->GetAuthGameMode<ACoopGameGameModeBase>();
		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}

}

void USHealthComponent::Heal_Implementation(float HealAmount)
{
	if (HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);

	// debug
	FString OwnerName;
	
	AActor* Owner = GetOwner();
	if (Owner != nullptr) OwnerName = Owner->GetName();
	
	FString Msg = FString::Printf(TEXT("%s Health Changed: %s (+%s)"), *OwnerName, *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount));
	
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta, Msg);
	//
}


void USHealthComponent::OnRep_Health()
{
	// UE_LOG(LogTemp, Warning, TEXT("OH HEALTH BROADCAST"));
	OnHealthChangedClient.Broadcast();
}


void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);

}
