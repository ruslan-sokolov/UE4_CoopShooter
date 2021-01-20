// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon_Projectile.h"
#include "DrawDebugHelpers.h"

#include "SProjectile.h"
#include "Kismet/GameplayStatics.h"

ASWeapon_Projectile::ASWeapon_Projectile()
{

	// Default Projectile To Spawn
	Projectile = ASProjectile::StaticClass();

	FireRate = 0.8f;
	FireMode = WeaponFireMode::Semiauto;
	bShotCanBeDelayed = false;
}

void ASWeapon_Projectile::FireLogic()
{
	APawn* Weapon_Owner = Cast<APawn>(GetOwner());

	if (Weapon_Owner) {

		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.Instigator = Weapon_Owner;
		ActorSpawnParams.Owner = this;

		FVector EyeLoc;
		FRotator EyeRot;
		Weapon_Owner->GetActorEyesViewPoint(EyeLoc, EyeRot);

		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ASProjectile* ProjectileInst = GetWorld()->SpawnActor<ASProjectile>(Projectile, MuzzleLoc, EyeRot, ActorSpawnParams);

	}

}
