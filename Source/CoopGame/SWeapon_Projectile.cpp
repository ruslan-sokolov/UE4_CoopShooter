// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon_Projectile.h"


ASWeapon_Projectile::ASWeapon_Projectile()
{
	FireLogicType = EWeaponFireLogicType::SpawnProjectile;
	FireRate = 0.8f;
	FireMode = EWeaponFireMode::Semiauto;
	bShotCanBeDelayed = false;
}
